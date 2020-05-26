#include "parameter_set_editor.hpp"
#include "spin_button_dim.hpp"
#include "common/common.hpp"

namespace horizon {

static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before)
{
    if (before && !row->get_header()) {
        auto ret = Gtk::manage(new Gtk::Separator);
        row->set_header(*ret);
    }
}

Gtk::Widget *ParameterSetEditor::create_extra_widget(ParameterID id)
{
    return nullptr;
}

class ParameterEditor : public Gtk::Box {
public:
    ParameterEditor(ParameterID id, ParameterSetEditor *pse)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8), parameter_id(id), parent(pse)
    {
        std::string parameter_name = pse->lookup_parameter_name(id);
        auto la = Gtk::manage(new Gtk::Label(parameter_name));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(1);
        parent->sg_label->add_widget(*la);
        pack_start(*la, false, false, 0);

        sp = Gtk::manage(new SpinButtonDim());
        sp->set_range(-1e9, 1e9);
        sp->set_value(parent->parameter_set->at(id));
        sp->signal_value_changed().connect([this] {
            (*parent->parameter_set)[parameter_id] = sp->get_value_as_int();
            parent->s_signal_changed.emit();
        });
        sp->signal_key_press_event().connect([this](GdkEventKey *ev) {
            if (ev->keyval == GDK_KEY_Return && (ev->state & GDK_SHIFT_MASK)) {
                sp->activate();
                parent->s_signal_apply_all.emit(parameter_id);
                parent->s_signal_changed.emit();
                return true;
            }
            return false;
        });
        sp->signal_activate().connect([this] {
            auto widgets = parent->listbox->get_children();
            auto my_row = sp->get_ancestor(GTK_TYPE_LIST_BOX_ROW);
            auto next = std::find(widgets.begin(), widgets.end(), my_row) + 1;
            if (next < widgets.end()) {
                auto e = dynamic_cast<ParameterEditor *>(dynamic_cast<Gtk::ListBoxRow *>(*next)->get_child());
                e->sp->grab_focus();
            }
            else {
                parent->s_signal_activate_last.emit();
            }
        });
        pack_start(*sp, true, true, 0);

        auto delete_button = Gtk::manage(new Gtk::Button());
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        pack_start(*delete_button, false, false, 0);
        delete_button->signal_clicked().connect([this] {
            parent->erase_cb(parameter_id);
            parent->parameter_set->erase(parameter_id);
            parent->s_signal_changed.emit();
            delete this->get_parent();
        });

        if (auto extra_button = parent->create_extra_widget(id)) {
            pack_start(*extra_button, false, false, 0);
        }


        set_margin_start(4);
        set_margin_end(4);
        set_margin_top(4);
        set_margin_bottom(4);
        show_all();
    }
    SpinButtonDim *sp = nullptr;

    const ParameterID parameter_id;

private:
    ParameterSetEditor *parent;
};

ParameterSetEditor::ParameterSetEditor(std::variant<ParameterDef *, const ParameterDef *> pd, ParameterSet *ps,
                                       bool populate_init, int)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), parameter_def(pd), parameter_set(ps)
{
    add_button = Gtk::manage(new Gtk::MenuButton());
    add_button->set_label("Add parameter");
    add_button->set_halign(Gtk::ALIGN_START);
    add_button->set_margin_bottom(10);
    add_button->set_margin_top(10);
    add_button->set_margin_start(10);
    add_button->set_margin_end(10);
    add_button->show();
    pack_start(*add_button, false, false, 0);


    auto popover = Gtk::manage(new Gtk::Popover());
    popover_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8));
    popover->add(*popover_box);
    add_button->set_popover(*popover);
    popover->signal_show().connect(sigc::mem_fun(*this, &ParameterSetEditor::update_popover_box));

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    listbox = Gtk::manage(new Gtk::ListBox());
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    listbox->set_header_func(&header_fun);
    sc->add(*listbox);

    sg_label = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    if (populate_init)
        populate();

    sc->show_all();
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }
    pack_start(*sc, true, true, 0);
}

ParameterSetEditor::ParameterSetEditor(const ParameterDef *pd, ParameterSet *ps, bool populate_init)
    : ParameterSetEditor(pd, ps, populate_init, 0)
{
}

ParameterSetEditor::ParameterSetEditor(ParameterDef *pd, ParameterSet *ps, bool populate_init)
    : ParameterSetEditor(pd, ps, populate_init, 0)
{
}

void ParameterSetEditor::set_button_margin_left(int margin)
{
    add_button->set_margin_start(margin);
}

void ParameterSetEditor::populate()
{
    for (auto &it : *parameter_set) {
        auto pe = Gtk::manage(new ParameterEditor(it.first, this));
        listbox->add(*pe);
    }
    listbox->show_all();
}

void ParameterSetEditor::focus_first()
{
    auto widgets = listbox->get_children();
    if (widgets.size()) {
        auto w = dynamic_cast<ParameterEditor *>(dynamic_cast<Gtk::ListBoxRow *>(widgets.front())->get_child());
        w->sp->grab_focus();
    }
}

void ParameterSetEditor::add_or_set_parameter(ParameterID param, int64_t value)
{
    if (parameter_set->count(param)) {
        auto rows = listbox->get_children();
        for (auto row : rows) {
            auto w = dynamic_cast<ParameterEditor *>(dynamic_cast<Gtk::ListBoxRow *>(row)->get_child());
            if (w->parameter_id == param)
                w->sp->set_value(value);
        }
    }
    else {
        (*parameter_set)[param] = value;
        auto pe = Gtk::manage(new ParameterEditor(param, this));
        listbox->add(*pe);
    }
    s_signal_changed.emit();
}

const ParameterDef *ParameterSetEditor::get_parameter_def() const
{
    if (auto def = std::get_if<const ParameterDef *>(&this->parameter_def)) {
        return *def;
    }
    else {
        return std::get<ParameterDef *>(this->parameter_def);
    }
}

ParameterDef *ParameterSetEditor::get_mutable_parameter_def()
{
    if (auto def = std::get_if<ParameterDef *>(&this->parameter_def)) {
        return *def;
    }
    else {
        abort();
    }
}

std::string ParameterSetEditor::lookup_parameter_name(ParameterID parameter) const
{
    std::string name;
    if (auto parameter_name = get_parameter_def()->lookup_name(parameter)) {
        name = *parameter_name;
    }
    else {
        name = "<unknown custom parameter '";
        name.append(parameter.id());
        name += "'>";
    }

    return name;
}

void ParameterSetEditor::update_popover_box()
{
    for (auto it : popover_box->get_children()) {
        delete it;
    }

    bool add_custom;
    const ParameterDef *pd;
    if (auto def = std::get_if<const ParameterDef *>(&this->parameter_def)) {
        pd = *def;
        add_custom = false;
    }
    else {
        pd = std::get<ParameterDef *>(this->parameter_def);
        add_custom = true;
    }

    // TODO: Implement adding custom parameters.
    static_cast<void>(add_custom);

    for (const auto &parameter : pd->parameters()) {
        if (parameter_set->count(parameter) == 0) {
            std::string name = lookup_parameter_name(parameter);
            auto bu = Gtk::manage(new Gtk::Button(name));
            bu->signal_clicked().connect([this, parameter] {
                auto popover = dynamic_cast<Gtk::Popover *>(popover_box->get_ancestor(GTK_TYPE_POPOVER));
#if GTK_CHECK_VERSION(3, 22, 0)
                popover->popdown();
#else
                popover->hide();
#endif
                (*parameter_set)[parameter] = 0.5_mm;
                auto pe = Gtk::manage(new ParameterEditor(parameter, this));
                listbox->add(*pe);
                s_signal_changed.emit();
            });
            popover_box->pack_start(*bu, false, false, 0);
        }
    }
    popover_box->show_all();
}
} // namespace horizon
