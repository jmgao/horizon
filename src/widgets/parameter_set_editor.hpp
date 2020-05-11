#pragma once
#include <gtkmm.h>
#include <variant>
#include "parameter/set.hpp"
#include "util/changeable.hpp"

namespace horizon {
class ParameterSetEditor : public Gtk::Box, public Changeable {
    friend class ParameterEditor;

private:
    ParameterSetEditor(std::variant<ParameterDef *, const ParameterDef *> pd, ParameterSet *ps, bool populate_init,
                       int dummy);

public:
    ParameterSetEditor(const ParameterDef *pd, ParameterSet *ps, bool populate_init = false);
    ParameterSetEditor(ParameterDef *pd, ParameterSet *ps, bool populate_init = false);
    void populate();
    void focus_first();
    void set_button_margin_left(int margin);
    void add_or_set_parameter(ParameterID param, int64_t value);

    typedef sigc::signal<void> type_signal_activate_last;
    type_signal_activate_last signal_activate_last()
    {
        return s_signal_activate_last;
    }

    typedef sigc::signal<void, ParameterID> type_signal_apply_all;
    type_signal_apply_all signal_apply_all()
    {
        return s_signal_apply_all;
    }

protected:
    virtual Gtk::Widget *create_extra_widget(ParameterID id);
    virtual void erase_cb(ParameterID id)
    {
    }


    std::string lookup_parameter_name(ParameterID parameter) const;
    const ParameterDef *get_parameter_def() const;
    ParameterDef *get_mutable_parameter_def();

private:
    std::variant<ParameterDef *, const ParameterDef *> parameter_def;

protected:
    ParameterSet *parameter_set;

private:
    Gtk::MenuButton *add_button = nullptr;
    Gtk::ListBox *listbox = nullptr;
    Gtk::Box *popover_box = nullptr;
    Glib::RefPtr<Gtk::SizeGroup> sg_label;
    void update_popover_box();

    type_signal_activate_last s_signal_activate_last;

protected:
    type_signal_apply_all s_signal_apply_all;
};
} // namespace horizon
