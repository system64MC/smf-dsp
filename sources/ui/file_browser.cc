//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ui/file_browser.h"
#include "ui/color_palette.h"
#include "ui/fonts.h"
#include "ui/text.h"
#include "ui/paint.h"
#include "utility/charset.h"
#include "utility/SDL++.h"
#include <gsl.hpp>
#include <algorithm>
#include <cstdlib>
#include <cassert>

static constexpr unsigned fn_name_chars = 24;
static constexpr unsigned fn_max_chars = fn_name_chars + 2;

File_Browser::File_Browser(const Rect &bounds)
    : FileOpenCallback(model_.FileOpenCallback),
      FileFilterCallback(model_.FileFilterCallback),
      bounds_(bounds)
{
    set_font(&font_s14);
    focus_on_selection();
}

unsigned File_Browser::current_index() const
{
    return model_.selection();
}

unsigned File_Browser::current_count() const
{
    return model_.count();
}

const File_Entry *File_Browser::current_entry() const noexcept
{
    return model_.current_entry();
}

std::string File_Browser::current_path() const
{
    return model_.current_path();
}

void File_Browser::set_current_path(const std::string &path)
{
    model_.set_current_path(path);
    focus_on_selection();
}

void File_Browser::set_font(Font *font)
{
    font_ = font;
    focus_on_selection();
}

const std::string &File_Browser::cwd() const
{
    return model_.cwd();
}

void File_Browser::set_cwd(const std::string &dir)
{
    model_.set_cwd(dir);
    focus_on_selection();
}

void File_Browser::paint(SDL_Renderer *rr)
{
    Rect bounds = bounds_;
    const Color_Palette &pal = Color_Palette::get_current();

    size_t rows = this->rows();
    size_t cols = this->cols();
    size_t iw = item_width();
    size_t ih = item_height();

    size_t nent = model_.count();
    size_t sel = model_.selection();

    Text_Painter tp;
    tp.rr = rr;
    tp.font = font_;
    int fw = tp.font->width();

    size_t colindex = colindex_;

    for (size_t i = 0, n = rows * cols; i < n; ++i) {
        size_t entno = i + rows * colindex;
        if (entno >= nent)
            break;

        const File_Entry &entry = model_.entry(entno);

        const std::string &file_name = entry.name;

        std::string name_drawn;
        if (entry.type == 'D')
            name_drawn = '/' + file_name;
        else
            name_drawn = file_name;

        long padchars = ((long)(fn_name_chars * fw) - (long)tp.measure_utf8(name_drawn)) / fw;
        if (padchars > 0)
            name_drawn.insert(name_drawn.end(), padchars, ' ');

        size_t r = i % rows;
        size_t c = i / rows;

        Rect ib(bounds.x + c * iw, bounds.y + r * ih, fn_name_chars * fw, ih);
        tp.pos.x = ib.x;
        tp.pos.y = ib.y;

        tp.fg = pal[Colors::text_browser_foreground];
        if (entno == sel) {
            tp.fg = pal[Colors::info_box_background];
            SDLpp_SetRenderDrawColor(rr, pal[Colors::text_browser_foreground]);
            SDL_RenderFillRect(rr, &ib);
        }

        SDLpp_ClipState clip;
        SDLpp_SaveClipState(rr, clip);
        SDL_RenderSetClipRect(rr, &ib);
        tp.draw_utf8(name_drawn);
        SDLpp_RestoreClipState(rr, clip);
    }
}

bool File_Browser::handle_key_pressed(const SDL_KeyboardEvent &event)
{
    int keymod = event.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI);
    if (keymod == KMOD_NONE) {
        switch (event.keysym.scancode) {
        case SDL_SCANCODE_UP:
            move_selection_by(-1);
            return true;
        case SDL_SCANCODE_DOWN:
            move_selection_by(+1);
            return true;
        case SDL_SCANCODE_LEFT:
            move_selection_by(-rows());
            return true;
        case SDL_SCANCODE_RIGHT:
            move_selection_by(+rows());
            return true;
        case SDL_SCANCODE_RETURN:
            model_.trigger_entry(model_.selection());
            focus_on_selection();
            return true;
        case SDL_SCANCODE_BACKSPACE:
            model_.trigger_entry(model_.find_entry(".."));
            focus_on_selection();
            return true;
        default:
            break;
        }
    }

    return false;
}

bool File_Browser::handle_key_released(const SDL_KeyboardEvent &event)
{
    return false;
}

void File_Browser::move_selection_by(long offset)
{
    model_.set_selection((long)model_.selection() + offset);
    focus_on_selection();
}

void File_Browser::focus_on_selection()
{
    size_t sel = model_.selection();
    size_t count = model_.count();
    size_t rows = this->rows();
    size_t cols = this->cols();
    size_t totalcols = (count + rows - 1) / rows;
    size_t selcol = sel / rows;
    size_t maxcolindex = std::max((long)totalcols - (long)cols, 0l);
    colindex_ = std::min(selcol - std::min(cols / 2, selcol), maxcolindex);
}

size_t File_Browser::item_width() const
{
    return fn_max_chars * font_->width();
}

size_t File_Browser::item_height() const
{
    return font_->height();
}

size_t File_Browser::rows() const
{
    return std::max<size_t>(1, bounds_.h / item_height());
}

size_t File_Browser::cols() const
{
    return std::max<size_t>(1, bounds_.w / item_width());
}
