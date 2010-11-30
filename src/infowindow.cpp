/*
 * Toonloop
 *
 * Copyright 2010 Alexandre Quessy
 * <alexandre@quessy.net>
 * http://www.toonloop.com
 *
 * Toonloop is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Toonloop is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the gnu general public license
 * along with Toonloop.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <clutter/clutter.h>
#include "application.h"
#include "clip.h"
#include "gui.h"
#include "infowindow.h"
#include "pipeline.h"

/**
 * Window to display some information. 
 */
InfoWindow::InfoWindow(Application *app) : 
    app_(app),
    stage_(NULL),
    text_(NULL)
{
}
void InfoWindow::create()
{
    stage_ = clutter_stage_new();
    if (! stage_)
    {
        g_critical("Could not get a stage. The Clutter backend might not support multiple stages.");
    }
    else
    {
        ClutterColor black = {0, 0, 0, 255};
        ClutterColor white = {255, 255, 255, 255};
        clutter_stage_set_color(CLUTTER_STAGE(stage_), &black);
        clutter_stage_set_title(CLUTTER_STAGE(stage_), "Toonloop Information");
        // TODO:make stage's size configurable
        gfloat width = 320.0f;
        gfloat height = 240.0f;
        clutter_actor_set_size(stage_, width, height);
        text_ = clutter_text_new_full("Sans semibold 12px", "", &white);
        clutter_container_add_actor(CLUTTER_CONTAINER(stage_), text_);
        clutter_actor_show(stage_);
    }
}

void InfoWindow::update_info_window()
{
    if (! text_)
        return;
    Gui *gui = app_->get_gui();
    Clip* current_clip = app_->get_current_clip();
    Gui::layout_number current_layout = gui->get_layout();
    Gui::BlendingMode blending_mode = gui->get_blending_mode();
    std::ostringstream os;

    os << "Layout: " << current_layout << " (" << gui->get_layout_name(current_layout) << ")" << std::endl;
    os << "Blending mode: " << blending_mode << " (" << gui->get_blending_mode_name(blending_mode) << ")" << std::endl;
    os << std::endl;
    os << "CLIP: " << current_clip->get_id() << std::endl;
    os << "  FPS: " << current_clip->get_playhead_fps() << std::endl;
    os << "  Playhead: " << current_clip->get_playhead() << std::endl;
    os << "  Writehead: " << current_clip->get_writehead() << "/" << current_clip->size() << std::endl;
    os << "  Direction: " << Clip::get_direction_name(current_clip->get_direction()) << std::endl;
    os << std::endl;
    os << "  Intervalometer rate: " << current_clip->get_intervalometer_rate() << std::endl;
    os << "  Intervalometer enabled: " << (app_->get_pipeline()->get_intervalometer_is_on() ? "yes" : "no") << std::endl;
    clutter_text_set_text(CLUTTER_TEXT(text_), os.str().c_str());
}