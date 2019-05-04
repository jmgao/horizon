#include "welcome_window.hpp"
#include "pool-prj-mgr-app_win.hpp"
#include "pool-prj-mgr-app.hpp"

namespace horizon {
WelcomeWindow *WelcomeWindow::create(PoolProjectManagerAppWindow *aw)
{
    WelcomeWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/welcome.ui");
    x->get_widget_derived("window", w, aw);

    w->set_transient_for(*aw);

    return w;
}

WelcomeWindow::WelcomeWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                             PoolProjectManagerAppWindow *aw)
    : Gtk::Window(cobject), appwin(aw)
{
    Gtk::Button *button_download = nullptr;
    Gtk::Button *button_open = nullptr;
    Gtk::Button *button_git = nullptr;

    x->get_widget("banner_area", banner_area);
    x->get_widget("button_download", button_download);
    x->get_widget("button_open", button_open);
    x->get_widget("button_git", button_git);
    x->get_widget("button_back", button_back);
    x->get_widget("button_git_add_pool", button_git_add_pool);
    x->get_widget("stack", stack);
    button_back->set_visible(false);

    pixbuf = Gdk::Pixbuf::create_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/welcome-banner.jpg");
    banner_area->signal_draw().connect(sigc::mem_fun(*this, &WelcomeWindow::draw_banner));

    button_download->signal_clicked().connect([this] {
        appwin->handle_download();
        delete this;
    });

    button_open->signal_clicked().connect(sigc::mem_fun(*this, &WelcomeWindow::handle_open));
    button_git_add_pool->signal_clicked().connect(sigc::mem_fun(*this, &WelcomeWindow::handle_open));

    button_git->signal_clicked().connect([this] {
        stack->set_visible_child("git");
        button_back->set_visible(true);
    });

    button_back->signal_clicked().connect([this] {
        stack->set_visible_child("start");
        button_back->set_visible(false);
    });
}

void WelcomeWindow::handle_open()
{
    auto app = Glib::RefPtr<PoolProjectManagerApplication>::cast_dynamic(appwin->get_application());
    app->show_preferences_window(0, PoolProjectManagerApplication::PreferencesWindowAction::OPEN_POOL);
    delete this;
}

bool WelcomeWindow::draw_banner(const Cairo::RefPtr<Cairo::Context> &cr)
{
    auto alloc = banner_area->get_allocation();
    double w = pixbuf->get_width();
    double h = pixbuf->get_height();
    auto scale = alloc.get_width() / w;

    cr->set_source_rgb(0.012, 0.075, 0.173);
    cr->paint();

    auto sc = pixbuf->scale_simple(w * scale, h * scale, Gdk::INTERP_BILINEAR);
    Gdk::Cairo::set_source_pixbuf(cr, sc);
    cr->paint();
    return true;
}
} // namespace horizon
