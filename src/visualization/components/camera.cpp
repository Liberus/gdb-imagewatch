/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2017 GDB ImageWatch contributors
 * (github.com/csantosbh/gdb-imagewatch/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <cmath>

#include <GL/glew.h>

#include "camera.h"

#include "ui/gl_canvas.h"
#include "visualization/game_object.h"
#include "visualization/stage.h"


Camera& Camera::operator=(const Camera& cam)
{
    zoom_power_    = cam.zoom_power_;
    camera_pos_x_  = cam.camera_pos_x_;
    camera_pos_y_  = cam.camera_pos_y_;
    canvas_width_  = cam.canvas_width_;
    canvas_height_ = cam.canvas_height_;
    scale_         = cam.scale_;

    update_object_pose();

    return *this;
}


void Camera::window_resized(int w, int h)
{
    projection.set_ortho_projection(w / 2.0, h / 2.0, -1.0f, 1.0f);
    canvas_width_  = w;
    canvas_height_ = h;
}


void Camera::scroll_callback(float delta)
{
    float mouse_x = gl_canvas->mouse_x();
    float mouse_y = gl_canvas->mouse_y();
    float win_w   = gl_canvas->width();
    float win_h   = gl_canvas->height();

    vec4 mouse_pos_ndc(2.0 * (mouse_x - win_w / 2) / win_w,
                       -2.0 * (mouse_y - win_h / 2) / win_h,
                       0,
                       1);
    mat4 vp_inv = game_object->get_pose() * projection.inv();

    float delta_zoom = std::pow(zoom_factor, -delta);

    vec4 mouse_pos = scale_.inv() * vp_inv * mouse_pos_ndc;

    // Since the view matrix of the camera is inverted before being applied to
    // the world coordinates, the order in which the operations below are
    // applied to world coordinates during rendering will also be reversed

    // clang-format off
    scale_ = scale_ *
             mat4::translation(mouse_pos) *
             mat4::scale(vec4(delta_zoom, delta_zoom, 1.0, 1.0)) *
             mat4::translation(-mouse_pos);
    // clang-format on

    // Calls to compute_zoom will require the zoom_power_ parameter to be on par
    // with the accumulated delta_zooms
    zoom_power_ += delta;

    update_object_pose();
}


void Camera::update()
{
}


void Camera::update_object_pose()
{
    if (game_object != nullptr) {
        vec4 position{-camera_pos_x_, -camera_pos_y_, 0, 1.0f};

        // Since the view matrix of the camera is inverted before being applied
        // to the world coordinates, the order in which the operations below are
        // applied to world coordinates during rendering will also be reversed

        // clang-format off
        mat4 pose =  scale_ *
                     mat4::translation(position);
        // clang-format on

        game_object->set_pose(pose);
    }
}


bool Camera::post_initialize()
{
    window_resized(gl_canvas->width(), gl_canvas->height());
    set_initial_zoom();
    update_object_pose();

    return true;
}


void Camera::set_initial_zoom()
{
    GameObject* buffer_obj = game_object->stage->get_game_object("buffer");
    Buffer* buff = buffer_obj->get_component<Buffer>("buffer_component");

    vec4 buf_dim = buffer_obj->get_pose() *
                   vec4(buff->buffer_width_f, buff->buffer_height_f, 0, 1);

    zoom_power_ = 0.0;

    if (canvas_width_ > buf_dim.x() && canvas_height_ > buf_dim.y()) {
        // Zoom in
        zoom_power_ += 1.0;
        float new_zoom = compute_zoom();

        // Iterate until buffer can fit inside the canvas
        while (canvas_width_ > new_zoom * buf_dim.x() &&
               canvas_height_ > new_zoom * buf_dim.y()) {
            zoom_power_ += 1.0;
            new_zoom = compute_zoom();
        }

        zoom_power_ -= 1.0;
    } else if (canvas_width_ < buf_dim.x() || canvas_height_ < buf_dim.y()) {
        // Zoom out
        zoom_power_ -= 1.0;
        float new_zoom = compute_zoom();

        // Iterate until buffer can fit inside the canvas
        while (canvas_width_ < new_zoom * buf_dim.x() ||
               canvas_height_ < new_zoom * buf_dim.y()) {
            zoom_power_ -= 1.0;
            new_zoom = compute_zoom();
        }
    }

    float zoom = 1.f / compute_zoom();
    scale_     = mat4::scale(vec4(zoom, zoom, 1.0, 1.0));
}


float Camera::compute_zoom()
{
    return std::pow(zoom_factor, zoom_power_);
}


void Camera::recenter_camera()
{
    camera_pos_x_ = camera_pos_y_ = 0.0f;

    set_initial_zoom();
    update_object_pose();
}


void Camera::mouse_drag_event(int mouse_x, int mouse_y)
{
    // Mouse is down. Update camera_pos_x_/camera_pos_y_
    camera_pos_x_ += mouse_x;
    camera_pos_y_ += mouse_y;

    update_object_pose();
}


bool Camera::post_buffer_update()
{
    return true;
}
