#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

//#include <biji.hpp>   
#include <signals/signals.hpp>
#include <cppurses/cppurses.hpp>

#include "dark_menu.hpp"

//using namespace biji;
namespace cppurses {

Dark_menu::Dark_menu(Glyph_string title_text)
    : title{this->make_child<Label>(std::move(title_text))}
{
    focus_policy = Focus_policy::Direct;
    title.set_alignment(Alignment::Center);
    hide_line_break();
    hide_title();
    set_name("dark_menu");
}

sig::Signal<void()>& Dark_menu::append_item(Glyph_string label)
{
    return insert_item(std::move(label), size());
}

sig::Signal<void()>& Dark_menu::insert_item(Glyph_string label, std::size_t index)
{
    auto button_ptr = std::make_unique<Push_button>(std::move(label));
    Push_button& new_button = *button_ptr;
    new_button.set_name("new button");
    children.insert(std::move(button_ptr), index + 2);
    items_.emplace(std::begin(items_) + index, new_button);
    //items_.set_name("menu items vector");
    new_button.install_event_filter(*this);
    new_button.height_policy.fixed(1);

    if (items_.size() == 1)
    {
        this->select_item(0);
    }
    new_button.clicked.connect([this, index]
    {
        this->select_item(index);
        this->send_selected_signal();
    });
    return items_[index].selected;

}

void Dark_menu::remove_item(std::size_t index)
{
    if (index >= items_.size())
    {
        return;
    }
    items_[index].button.get().close();
    items_.erase(std::begin(items_) + index);
    if (index == selected_index_)
    {
        this->select_item(0);
    }
}

void Dark_menu::select_up(std::size_t n)
{
   if (Focus::focus_widget())
        spdlog::debug("Focus is: {}", Focus::focus_widget()->name());
    else
        spdlog::debug("None"); 
    const auto new_index = selected_index_ > n ? selected_index_ - n : 0;
    this->select_item(new_index);
    spdlog::debug("select up function called");
}

void Dark_menu::select_down(std::size_t n)
{
   if (Focus::focus_widget())
        spdlog::debug("Focus is: {}", Focus::focus_widget()->name());
    else
        spdlog::debug("None"); 
    this->select_item(selected_index_ + n);
    spdlog::debug("select down function called");
}

void Dark_menu::select_item(std::size_t index)
{
   if (Focus::focus_widget())
        spdlog::debug("Focus is: {}", Focus::focus_widget()->name());
    else
        spdlog::debug("None"); 

    if (items_.empty())
    {
        return;
        spdlog::debug("vector is empty");
    }

    auto& previous_btn = items_[selected_index_].button.get();
    previous_btn.set_name("previous button");
    previous_btn.brush.remove_attributes(selected_attr_);
    previous_btn.update();

    selected_index_ = index >= items_.size() ? items_.size() - 1 : index;

    auto& current_btn = items_[selected_index_].button.get();
    current_btn.set_name("current button");
    current_btn.brush.add_attributes(selected_attr_);
    current_btn.update();
}

void Dark_menu::set_selected_attribute(const Attribute& attr)
{
    auto& selected_btn = items_[selected_index_].button.get();
    selected_btn.set_name("selected button");
    selected_btn.brush.remove_attributes(selected_attr_);
    selected_attr_ = attr;
    selected_btn.brush.add_attributes(selected_attr_);
    selected_btn.update();
}

void Dark_menu::hide_title()
{
    title_enabled_ = false;
    this->enable(this->enabled());
}

void Dark_menu::show_title()
{
    title_enabled_ = true;
    this->enable(this->enabled());
}

void Dark_menu::hide_line_break()
{
    line_break_enabled_ = false;
    this->enable(this->enabled());
}

void Dark_menu::show_line_break()
{
    line_break_enabled_ = true;
    this->enable(this->enabled());
}

void Dark_menu::enable(bool enable, bool post_child_polished_event)
{
    this->enable_and_post_events(enable, post_child_polished_event);
    line_break.enable(line_break_enabled_ && enable, post_child_polished_event);
    title.enable(title_enabled_ && enable, post_child_polished_event);
    for (Dark_menu_item& item : items_)
    {
        item.button.get().enable(enable, post_child_polished_event);
    }
}

bool Dark_menu::key_press_event(const Key::State& keyboard)
{
   if (Focus::focus_widget())
        spdlog::debug("Focus is: {}", Focus::focus_widget()->name());
    else
        spdlog::debug("None"); 
    spdlog::debug("Dark menu key press called");
    if (keyboard.key == Key::Arrow_right || keyboard.key == Key::l)
    {
        select_down();
    } else if (keyboard.key == Key::Arrow_left || keyboard.key == Key::h)
    {
        select_up();
    } else if (keyboard.key == Key::Enter)
    {
        spdlog::debug("Enter pushed");
        send_selected_signal();
    }
    return true;
}

bool Dark_menu::mouse_press_event(const Mouse::State& mouse)
{
    if (mouse.button == Mouse::Button::ScrollUp)
    {
        select_up();
    }
    else if (mouse.button == Mouse::Button::ScrollDown)
    {
        select_down();
    }
    return layout::Horizontal::mouse_press_event(mouse);
}

bool Dark_menu::mouse_press_event_filter(Widget& /* receiver */,
                                    const Mouse::State& mouse)
{
    if (mouse.button == Mouse::Button::ScrollUp)
    {
        this->select_up();
        return true;
    } else if (mouse.button == Mouse::Button::ScrollDown)
    {
        this->select_down();
        return true;
    }
    return false;
}

void Dark_menu::send_selected_signal()
{
    if (!items_.empty())
    {
        items_[selected_index_].selected();
        spdlog::debug("Signal sent");
    if (Focus::focus_widget())
        spdlog::debug("Focus is: {}", Focus::focus_widget()->name());
    else
        spdlog::debug("None"); 
    }
}

namespace slot {

sig::Slot<void(std::size_t)> select_up(Dark_menu& m)
{
    sig::Slot<void(std::size_t)> slot{[&m](auto n) { m.select_up(n); }};
    slot.track(m.destroyed);
    return slot;
}

sig::Slot<void()> select_up(Dark_menu& m, std::size_t n)
{
    sig::Slot<void()> slot{[&m, n] { m.select_up(n); }};
    slot.track(m.destroyed);
    return slot;
}

sig::Slot<void(std::size_t)> select_down(Dark_menu& m)
{
    sig::Slot<void(std::size_t)> slot{[&m](auto n) { m.select_down(n); }};
    slot.track(m.destroyed);
    return slot;
}

sig::Slot<void()> select_down(Dark_menu& m, std::size_t n)
{
    sig::Slot<void()> slot{[&m, n] { m.select_down(n); }};
    slot.track(m.destroyed);
    return slot;
}

sig::Slot<void(std::size_t)> select_item(Dark_menu& m)
{
    sig::Slot<void(std::size_t)> slot{
        [&m](auto index) { m.select_item(index); }};
    slot.track(m.destroyed);
    return slot;
}

sig::Slot<void()> select_item(Dark_menu& m, std::size_t index)
{
    sig::Slot<void()> slot{[&m, index] { m.select_item(index); }};
    slot.track(m.destroyed);
    return slot;
}
}  // namespace slot
}


