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
#include <boost/filesystem.hpp>
#include <clutter-gst/clutter-gst.h>
#include <clutter-gtk/clutter-gtk.h>
#include <clutter/clutter.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gst/gst.h>
#include <gtk/gtk.h>
#include <iostream>

#include "application.h"
#include "clip.h"
#include "config.h"
#include "controller.h"
#include "gui.h"
#include "pipeline.h"
#include "timer.h"

namespace fs = boost::filesystem;

gboolean Gui::on_window_state_event(GtkWidget* /*widget*/, GdkEventWindowState *event, gpointer user_data)
{
    Gui *context = static_cast<Gui*>(user_data);
    context->isFullscreen_ = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN);
    if (context->isFullscreen_)
        context->hideCursor();
    else
        context->showCursor();
    return TRUE;
}

void Gui::hideCursor()
{
    // FIXME: this is because gtk doesn't support GDK_BLANK_CURSOR before gtk-2.16
    // FIXME:2010-08-06:aalex:Hiding the cursor is currently broken
    char invisible_cursor_bits[] = { 0x0 };
    static GdkCursor* cursor = 0;
    if (cursor == 0)
    {
        static GdkBitmap *empty_bitmap;
        const static GdkColor color = {0, 0, 0, 0};
        empty_bitmap = gdk_bitmap_create_from_data(GDK_WINDOW(clutter_widget_->window), invisible_cursor_bits, 1, 1);
        cursor = gdk_cursor_new_from_pixmap(empty_bitmap, empty_bitmap, &color, &color, 0, 0);
    }
    gdk_window_set_cursor(GDK_WINDOW(clutter_widget_->window), cursor);
}

void Gui::showCursor()
{
    /// sets to default
    gdk_window_set_cursor(GDK_WINDOW(clutter_widget_->window), NULL);
}

/**
 * Handles key pressed event.
 *
 * Up: increase playhead FPS
 * Down: decrease playhead FPS
 * Backscape: remove a frame
 * Escape or f: toggle full screen mode
 * Space: add a frame
 * Page Up: choose next clip
 * Page Down: choose previous clip
 * 0, 1, 2, 3, 4, 5, 6, 7, 8, 9: choose a clip
 * Ctrl-q: quit
 * Ctrl-s: save
 */

gboolean Gui::key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    Gui *context = static_cast<Gui*>(user_data);
    Clip *current_clip = context->owner_->get_current_clip();
    unsigned int clip;

    switch (event->keyval)
    {
        case GDK_Up:
            current_clip->increase_playhead_fps();
            break;
        case GDK_Down:
            current_clip->decrease_playhead_fps();
            break;
        //case GDK_Left:
        //case GDK_Right:
        //case GDK_Return:
        case GDK_BackSpace:
            context->owner_->get_pipeline()->remove_frame();
            break;
        case GDK_f:
        case GDK_Escape:
            context->toggleFullscreen(widget);
            break;
        case GDK_space:
            context->owner_->get_pipeline()->grab_frame();
            break;
        case GDK_Page_Up:
            clip = context->owner_->get_current_clip_number();
            if (clip < MAX_CLIPS - 1)
            {
                context->owner_->set_current_clip_number(clip + 1);
                // Not needed, but we never know:
                current_clip = context->owner_->get_current_clip();
            }
            break;
        case GDK_Page_Down:
            clip = context->owner_->get_current_clip_number();
            if (clip > 0)
            {
                context->owner_->set_current_clip_number(clip - 1);
                // Not needed, but we never know:
                current_clip = context->owner_->get_current_clip();
            }
            break;
        case GDK_0:
        case GDK_1:
        case GDK_2:
        case GDK_3:
        case GDK_4:
        case GDK_5:
        case GDK_6:
        case GDK_7:
        case GDK_8:
        case GDK_9:
            context->switch_to_clip_number(event->keyval);
            // Not needed, but we never know:
            current_clip = context->owner_->get_current_clip();
            break;
        case GDK_q:
            // Quit application on ctrl-q, this quits the main loop
            // (if there is one)
            if (event->state & GDK_CONTROL_MASK)
            {
                g_print("Ctrl-Q key pressed, quitting.\n");
                context->owner_->quit();
            }
            break;
        case GDK_s:
            // Ctrl-s: Saves the current clip, this quits the main loop
            // (if there is one)
            if (event->state & GDK_CONTROL_MASK)
            {
                g_print("Ctrl-S key pressed, saving.\n");
                context->owner_->save_current_clip();
            }
            break;
        default:
            break;
    }
    return TRUE;
}
/**
 * Switch the current clip according to a gdk key value from 0 to 9.
 *
 * keyval should be one of :
 * GDK_0 GDK_1 GDK_2 GDK_3 GDK_4 GDK_5 GDK_6 GDK_7 GDK_8 GDK_9
 * Of course, any other value might lead to a crash.
 */
void Gui::switch_to_clip_number(unsigned int key_val) 
{
    // FIXME:2010-08-17:aalex:Doing arithmetics with a gdk keyval is a hack
    unsigned int index = key_val & 0x0F;
    if (index > MAX_CLIPS)
        std::cout << "Invalid clip number " << index << std::endl;
    else {
        unsigned int clipIndex = Application::get_instance().get_current_clip_number();
        if (clipIndex != index) 
            Application::get_instance().set_current_clip_number(index);
    }
}

void Gui::on_delete_event(GtkWidget* /*widget*/, GdkEvent* /*event*/, gpointer /*data*/)
{
    //Gui *context = static_cast<Gui*>(data);
    g_print("Window has been deleted.\n");
    Application::get_instance().quit();
}

void Gui::toggleFullscreen(GtkWidget *widget)
{
    // toggle fullscreen state
    isFullscreen_ ? makeUnfullscreen(widget) : makeFullscreen(widget);
}

void Gui::makeFullscreen(GtkWidget *widget)
{
    gtk_window_stick(GTK_WINDOW(widget)); // window is visible on all workspaces
    gtk_window_fullscreen(GTK_WINDOW(widget));
}

void Gui::makeUnfullscreen(GtkWidget *widget)
{
    gtk_window_unstick(GTK_WINDOW(widget)); // window is not visible on all workspaces
    gtk_window_unfullscreen(GTK_WINDOW(widget));
}

void Gui::on_next_image_to_play(unsigned int /*clip_number*/, unsigned int/*image_number*/, std::string file_name)
{
    GError *error = NULL;
    gboolean success;
    success = clutter_texture_set_from_file(CLUTTER_TEXTURE(playback_texture_), file_name.c_str(), &error);
    // TODO: validate this path
    if (!success)
    {
        std::cerr << "Failed to load pixbuf file: " << file_name << " " << error->message << std::endl;
        g_error_free(error);
    } else {
        //std::cout << "Loaded image " <<  image_full_path << std::endl;
    }
}
/** 
 * Timeline handler.
 * Called on every frame. 
 *
 * Times the playback frames and display it if it's time to do so.
 *
 * Prints the rendering FPS information.
 * Calls Controller::update_playback_image
 */
void Gui::on_render_frame(ClutterTimeline * /*timeline*/, gint /*msecs*/, gpointer user_data)
{
    // Prints rendering FPS information
    static int number_of_frames_in_last_second = 0; // counting FPS
    static Timer fps_calculation_timer = Timer();
    
    Gui *context = static_cast<Gui*>(user_data);
    bool verbose = context->owner_->get_configuration()->get_verbose();
    Clip *thisclip = context->owner_->get_current_clip();

    fps_calculation_timer.tick();
    ++number_of_frames_in_last_second;
    // calculate rendering FPS
    if (fps_calculation_timer.get_elapsed() >= 1.0f)
    {
        if (verbose)
            std::cout << "Rendering FPS: " << number_of_frames_in_last_second << std::endl;
        number_of_frames_in_last_second = 0;
        fps_calculation_timer.reset();
    }

    context->owner_->get_controller()->update_playback_image();

    //TODO:2010-08-26:aalex:connect to Controller's on_no_image_to_play
    if(thisclip->size() > 0) 
    {     
        if (! CLUTTER_ACTOR_IS_VISIBLE(context->playback_texture_))
            clutter_actor_show_all(CLUTTER_ACTOR(context->playback_texture_));
    } else {
        if (CLUTTER_ACTOR_IS_VISIBLE(context->playback_texture_))
            clutter_actor_hide_all(CLUTTER_ACTOR(context->playback_texture_));
    }
}

/**
 * Called when the stage size has changed.
 */
void on_stage_allocation_changed(ClutterActor * /*stage*/, 
        ClutterActorBox * /*box*/, 
        ClutterAllocationFlags * /*flags*/, 
        gpointer user_data) 
{
    //g_print("on_stage_allocation_changed\n");
    Gui *gui = static_cast<Gui*>(user_data);
    gui->resize_actors();
}
/**
 * Called when it's time to resize the textures in the stage.
 *
 * Either the window has been resized, or the input image size has changed.
 */
void Gui::resize_actors() {
    // We could override the paint method of the stage
    // Or put everything in a container which has an apply_transform()
    gfloat set_x, set_y, set_width, set_height;
    gfloat stage_width, stage_height;

    clutter_actor_get_size(stage_, &stage_width, &stage_height);
    
    set_height = (video_input_height_ * stage_width) / video_input_width_;
    if (set_height <= stage_height) {
        set_width = stage_width;
        set_x = 0;
        set_y = (stage_height - set_height) / 2;
    } else {
        set_width  = (video_input_width_ * stage_height) / video_input_height_;
        set_height = stage_height;
        set_x = (stage_width - set_width) / 2;
        set_y = 0;
    }
    // Now that we know the ratio of stuff, 
    // Set the live texture size and position:
    gfloat live_tex_width = set_width / 2;
    gfloat live_tex_height = set_height / 2;
    gfloat live_tex_x = set_x;
    gfloat live_tex_y = (stage_height / 4);
    clutter_actor_set_position(CLUTTER_ACTOR(live_input_texture_), live_tex_x, live_tex_y);
    clutter_actor_set_size(CLUTTER_ACTOR(live_input_texture_), live_tex_width, live_tex_height);

    // Set the playback texture size and position:
    gfloat playback_tex_width = set_width / 2;
    gfloat playback_tex_height = set_height / 2;
    gfloat playback_tex_x = (stage_width / 2);
    gfloat playback_tex_y = (stage_height / 4);
    clutter_actor_set_position(CLUTTER_ACTOR(playback_texture_), playback_tex_x, playback_tex_y);
    clutter_actor_set_size(CLUTTER_ACTOR(playback_texture_), playback_tex_width, playback_tex_height);
}
/**
 * Called when the size of the input image has changed.
 */
void on_live_input_texture_size_changed(ClutterTexture *texture, gfloat width, gfloat height, gpointer user_data) {
    //g_print("on_live_input_texture_size_changed\n");
    Gui *gui = static_cast<Gui*>(user_data);
    gui->video_input_width_ = (float) width;
    gui->video_input_height_ = (float) height;

    ClutterActor *stage;
  
    stage = clutter_actor_get_stage(CLUTTER_ACTOR(texture));
    if (stage == NULL)
        return;
    gui->resize_actors();
}

void on_playback_texture_size_changed(ClutterTexture *texture, 
        gfloat /*width*/, gfloat /*height*/, gpointer user_data) 
{
    //g_print("on_playback_texture_size_changed\n");
    // TODO:2010-08-06:aalex:Take into account size and ratio of the playback texture
    Gui *gui = static_cast<Gui*>(user_data);
    ClutterActor *stage;
    stage = clutter_actor_get_stage(CLUTTER_ACTOR(texture));
    if (stage == NULL)
        return;
    gui->resize_actors();
}
/**
 * Exits the application if OpenGL needs are not met.
 */
Gui::Gui(Application* owner) :
    video_input_width_(1),
    video_input_height_(1),
    owner_(owner),
    isFullscreen_(false)
{
    //video_xwindow_id_ = 0;
    owner_->get_controller()->next_image_to_play_signal_.connect(boost::bind(&Gui::on_next_image_to_play, this, _1, _2, _3));
    //TODO: owner_->get_controller()->no_image_to_play_signals_.connect(boost::bind(&Gui::on_no_image_to_play, this))
    // Main GTK window
    window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    // TODO:2010-08-06:aalex:make window size configurable
    gtk_widget_set_size_request(window_, WINWIDTH, WINHEIGHT); 
    gtk_window_move(GTK_WINDOW(window_), 300, 10); // TODO: make configurable
    gtk_window_set_title(GTK_WINDOW(window_), std::string(std::string("Toonloop ") + std::string(PACKAGE_VERSION)).c_str());
    // Set window icon
    fs::path iconPath(std::string(PIXMAPS_DIR) + "/toonloop.png");
    if (fs::exists(iconPath))
        gtk_window_set_icon_from_file(GTK_WINDOW(window_), iconPath.string().c_str(), NULL);

    GdkGeometry geometry;
    geometry.min_width = 1;
    geometry.min_height = 1;
    geometry.max_width = -1;
    geometry.max_height = -1;
    gtk_window_set_geometry_hints(GTK_WINDOW(window_), window_, &geometry, GDK_HINT_MIN_SIZE);
    // connect window signals:
    g_signal_connect(G_OBJECT(window_), "delete-event", G_CALLBACK(on_delete_event), this);

    g_signal_connect(G_OBJECT(window_), "key-press-event", G_CALLBACK(key_press_event), this);
    // add listener for window-state-event to detect fullscreenness
    g_signal_connect(G_OBJECT(window_), "window-state-event", G_CALLBACK(on_window_state_event), this);

    // vbox:
    vbox_ = gtk_vbox_new(FALSE, 0); // args: homogeneous, spacing
    gtk_container_add(GTK_CONTAINER(window_), vbox_);

    //some buttons:
    //TODO:2010:08-17:aalex:Add a HBox with some buttons, plus a menu

    // Clutter widget:
    clutter_widget_ = gtk_clutter_embed_new();
    gtk_widget_set_size_request(clutter_widget_, WINWIDTH, WINHEIGHT);
    gtk_container_add(GTK_CONTAINER(vbox_), clutter_widget_);
    stage_ = gtk_clutter_embed_get_stage(GTK_CLUTTER_EMBED(clutter_widget_));

    clutter_stage_set_user_resizable(CLUTTER_STAGE(stage_), TRUE);
    g_signal_connect(stage_, "allocation-changed", G_CALLBACK(on_stage_allocation_changed), this);
    clutter_stage_set_minimum_size(CLUTTER_STAGE(stage_), 320, 240);

    /* We need to set certain props on the target texture currently for
     * efficient/corrent playback onto the texture (which sucks a bit)  
    */
    live_input_texture_ = (ClutterActor *) g_object_new(CLUTTER_TYPE_TEXTURE, 
        "sync-size", FALSE, 
        "disable-slicing", TRUE, 
        NULL);
    g_signal_connect(CLUTTER_TEXTURE(live_input_texture_), "size-change", G_CALLBACK(on_live_input_texture_size_changed), this);

    playback_texture_ = (ClutterActor *) g_object_new(CLUTTER_TYPE_TEXTURE, 
        "sync-size", FALSE, 
        "disable-slicing", TRUE, 
        NULL);
    g_signal_connect(CLUTTER_TEXTURE(playback_texture_), "size-change", G_CALLBACK(on_playback_texture_size_changed), this);

    // Background color:
    ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };
    clutter_stage_set_color(CLUTTER_STAGE(stage_), &stage_color);

    /* Create a timeline to manage animation */
    timeline_ = clutter_timeline_new(6000);
    g_object_set(timeline_, "loop", TRUE, NULL);   /* have it loop */
    /* fire a callback for frame change */
    g_signal_connect(timeline_, "new-frame", G_CALLBACK(on_render_frame), this);
    /* and start it */
    clutter_timeline_start(timeline_);

    /* of course, we need to show the texture in the stage. */

    clutter_container_add_actor(CLUTTER_CONTAINER(stage_), CLUTTER_ACTOR(live_input_texture_));
    clutter_container_add_actor(CLUTTER_CONTAINER(stage_), CLUTTER_ACTOR(playback_texture_));
  
    gtk_widget_show_all(window_);

    /* Only show the actors after parent show otherwise it will just be
     * unrealized when the clutter foreign window is set. widget_show
     * will call show on the stage.
     */
    clutter_actor_show_all(CLUTTER_ACTOR(live_input_texture_));
    //clutter_actor_show_all(CLUTTER_ACTOR(playback_texture_));
    if (owner_->get_configuration()->get_fullscreen())
        toggleFullscreen(window_);
}

ClutterActor* Gui::get_live_input_texture()
{
    return live_input_texture_;
}

