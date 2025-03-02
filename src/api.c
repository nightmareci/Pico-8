﻿/** @file api.c
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdint.h>
#include <wchar.h>
#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include "z8lua/fix32.h"
#include "config.h"

static SDL_Renderer* r;
static fix32_t seed_lo, seed_hi;
static Uint8 fill_mask[0x4000]; // Fill pattern mask.
static Uint64 seconds_since_start;

Uint8 pico8_ram[RAM_SIZE];

 /************************
  * Auxiliary functions. *
  ************************/

static void color_lookup(int col, Uint8* r, Uint8* g, Uint8* b)
{
    switch (col)
    {
        case 0: // black
            *r = 0x00;
            *g = 0x00;
            *b = 0x00;
            break;
        case 1: // dark-blue
            *r = 0x1d;
            *g = 0x2b;
            *b = 0x53;
            break;
        case 2: // dark-purple
            *r = 0x7e;
            *g = 0x25;
            *b = 0x53;
            break;
        case 3: // dark-green
            *r = 0x00;
            *g = 0x87;
            *b = 0x51;
            break;
        case 4: // brown
            *r = 0xab;
            *g = 0x52;
            *b = 0x36;
            break;
        case 5: // dark-grey
            *r = 0x5f;
            *g = 0x57;
            *b = 0x4f;
            break;
        case 6: // light-grey
        default:
            *r = 0xc2;
            *g = 0xc3;
            *b = 0xc7;
            break;
        case 7: // white
            *r = 0xff;
            *g = 0xf1;
            *b = 0xe8;
            break;
        case 8: // red
            *r = 0xff;
            *g = 0x00;
            *b = 0x4d;
            break;
        case 9: // orange
            *r = 0xff;
            *g = 0xa3;
            *b = 0x00;
            break;
        case 10: // yellow
            *r = 0xff;
            *g = 0xec;
            *b = 0x27;
            break;
        case 11: // green
            *r = 0x00;
            *g = 0xe4;
            *b = 0x36;
            break;
        case 12: // blue
            *r = 0x29;
            *g = 0xad;
            *b = 0xff;
            break;
        case 13: // lavender
            *r = 0x83;
            *g = 0x76;
            *b = 0x9c;
            break;
        case 14: // pink
            *r = 0xff;
            *g = 0x77;
            *b = 0xa8;
            break;
        case 15: // light-peach
            *r = 0xff;
            *g = 0xcc;
            *b = 0xaa;
            break;
        case -16: // brownish-black
        case 128:
            *r = 0x29;
            *g = 0x18;
            *b = 0x14;
            break;
        case -15: // darker-blue
        case 129:
            *r = 0x11;
            *g = 0x1d;
            *b = 0x35;
            break;
        case -14: // darker-purple
        case 130:
            *r = 0x42;
            *g = 0x21;
            *b = 0x36;
            break;
        case -13: // blue-green
        case 131:
            *r = 0x12;
            *g = 0x53;
            *b = 0x59;
            break;
        case -12: // dark-brown
        case 132:
            *r = 0x74;
            *g = 0x2f;
            *b = 0x29;
            break;
        case -11: // darker-grey
        case 133:
            *r = 0x49;
            *g = 0x33;
            *b = 0x3b;
            break;
        case -10: // medium-grey
        case 134:
            *r = 0xa2;
            *g = 0x88;
            *b = 0x79;
            break;
        case -9: // light-yellow
        case 135:
            *r = 0xf3;
            *g = 0xef;
            *b = 0x7d;
            break;
        case -8: // dark-red
        case 136:
            *r = 0xbe;
            *g = 0x12;
            *b = 0x50;
            break;
        case -7: // dark-orange
        case 137:
            *r = 0xff;
            *g = 0x6c;
            *b = 0x24;
            break;
        case -6: // lime-green
        case 138:
            *r = 0xa8;
            *g = 0xe7;
            *b = 0x2e;
            break;
        case -5: // medium-green
        case 139:
            *r = 0x00;
            *g = 0xb5;
            *b = 0x43;
            break;
        case -4: // true-blue
        case 140:
            *r = 0x06;
            *g = 0x5a;
            *b = 0xb5;
            break;
        case -3: // mauve
        case 141:
            *r = 0x75;
            *g = 0x46;
            *b = 0x65;
            break;
        case -2: // dark-peach
        case 142:
            *r = 0xff;
            *g = 0x6e;
            *b = 0x59;
            break;
        case -1: // peach
        case 143:
            *r = 0xff;
            *g = 0x9d;
            *b = 0x81;
            break;
    }
}

static void pset(int x, int y)
{
    if ((unsigned)x >= 128 || (unsigned)y >= 128)
    {
        return;
    }

    Uint32* fill_mask_ptr = (Uint32*)fill_mask;

    if (*fill_mask_ptr)
    {
        int index = y * 128 + x;
        if (fill_mask[index])
        {
            return;
        }
    }

    x += SCREEN_OFFSET_X;
    y += SCREEN_OFFSET_Y;

    SDL_RenderPoint(r, (float)x, (float)y);
}

static void draw_dot(int x, int y, int* color)
{
    if (!r)
    {
        return;
    }

    Uint8 r_set = 0x00, g_set = 0x00, b_set = 0x00;
    Uint8 r_prev, g_prev, b_rev, a_prev;

    SDL_GetRenderDrawColor(r, &r_prev, &g_prev, &b_rev, &a_prev);
    if (color)
    {
        color_lookup(*color, &r_set, &g_set, &b_set);
        SDL_SetRenderDrawColor(r, r_set, g_set, b_set, 255);
    }

    pset(x, y);

    if (color)
    {
        SDL_SetRenderDrawColor(r, r_prev, g_prev, b_rev, a_prev);
    }
}

static void draw_circle(int cx, int cy, int radius, int* color, bool fill)
{
    if (!r)
    {
        return;
    }

    Uint8 r_set, g_set, b_set;
    Uint8 r_prev, g_prev, b_rev, a_prev;

    SDL_GetRenderDrawColor(r, &r_prev, &g_prev, &b_rev, &a_prev);

    if (color)
    {
        color_lookup(*color, &r_set, &g_set, &b_set);
        SDL_SetRenderDrawColor(r, r_set, g_set, b_set, 255);
    }

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y)
    {
        if (fill)
        {
            for (int i = (cx - x); i <= (cx + x); i++)
            {
                pset(i, cy + y);
                pset(i, cy - y);
            }
            for (int i = (cx - y); i <= (cx + y); i++)
            {
                pset(i, cy + x);
                pset(i, cy - x);
            }
        }
        else
        {
            pset(cx + x, cy + y);
            pset(cx - x, cy + y);
            pset(cx + x, cy - y);
            pset(cx - x, cy - y);
            pset(cx + y, cy + x);
            pset(cx - y, cy + x);
            pset(cx + y, cy - x);
            pset(cx - y, cy - x);
        }

        if (d < 0)
        {
            d += 4 * x + 6;
        }
        else
        {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }

    if (color)
    {
        SDL_SetRenderDrawColor(r, r_prev, g_prev, b_rev, a_prev);
    }
}

static void draw_rect(int x0, int y0, int x1, int y1, int* color, bool fill)
{
   if (!r)
   {
       return;
   }

   Uint8 r_set, g_set, b_set;
   Uint8 r_prev, g_prev, b_rev, a_prev;
   SDL_GetRenderDrawColor(r, &r_prev, &g_prev, &b_rev, &a_prev);

   if (color)
   {
       color_lookup(*color, &r_set, &g_set, &b_set);
       SDL_SetRenderDrawColor(r, r_set, g_set, b_set, 255);
   }

   if (fill)
   {
       for (int y = y0; y <= y1; y++)
       {
           for (int x = x0; x <= x1; x++)
           {
               pset(x, y);
           }
       }
   }
   else
   {
       for (int x = x0; x <= x1; x++)
       {
           pset(x, y0);
           pset(x, y1);
       }
       for (int y = y0; y <= y1; y++)
       {
           pset(x0, y);
           pset(x1, y);
       }
   }

   if (color)
   {
       SDL_SetRenderDrawColor(r, r_prev, g_prev, b_rev, a_prev);
   }
}

static Uint8 peek(Uint16 addr)
{
    if (addr >= RAM_SIZE-1)
    {
        return 0;
    }
    return (pico8_ram[addr]);
}

static void poke(Uint16 addr, Uint8 data)
{
    if (addr >= RAM_SIZE-1)
    {
        return;
    }
    pico8_ram[addr] = data;
}

 /***************************
  * Flow-control functions. *
  ***************************/

static int pico8_time(lua_State* L)
{
    lua_pushnumber(L, fix32_from_uint64(seconds_since_start));
    return 1;
}

 /***********************
  * Graphics functions. *
  ***********************/

static int pico8_camera(lua_State* L)
{
    return 0;
}

static int pico8_circ(lua_State* L)
{
    int cx = fix32_to_int(luaL_checknumber(L, 1));
    int cy = fix32_to_int(luaL_checknumber(L, 2));
    int radius = fix32_to_int(luaL_optnumber(L, 3, fix32_value(4, 0)));
    int color;

    if (lua_gettop(L) == 4)
    {
        color = fix32_to_int(luaL_checkinteger(L, 4));
        draw_circle(cx, cy, radius, &color, false);
    }
    else
    {
        draw_circle(cx, cy, radius, NULL, false);
    }

    return 0;
}

static int pico8_circfill(lua_State* L)
{
    int cx = fix32_to_int(luaL_checknumber(L, 1));
    int cy = fix32_to_int(luaL_checknumber(L, 2));
    int radius = fix32_to_int(luaL_optnumber(L, 3, fix32_value(4, 0)));
    int color;

    if (lua_gettop(L) == 4)
    {
        color = fix32_to_int(luaL_checkinteger(L, 4));
        draw_circle(cx, cy, radius, &color, true);
    }
    else
    {
        draw_circle(cx, cy, radius, NULL, true);
    }

    return 0;
}

static int pico8_clip(lua_State* L)
{
    // Reset region when calling cls().
    return 0;
}

// Todo: Reset clip region, reset cursor position to 0,0.
static int pico8_cls(lua_State* L)
{
    int color = fix32_to_int32(luaL_optinteger(L, 1, 0));

    Uint8 r_set = 0x00, g_set = 0x00, b_set = 0x00;
    Uint8 r_prev, g_prev, b_rev, a_prev;

    SDL_GetRenderDrawColor(r, &r_prev, &g_prev, &b_rev, &a_prev);
    if (color)
    {
        color_lookup(color, &r_set, &g_set, &b_set);
        SDL_SetRenderDrawColor(r, r_set, g_set, b_set, 255);
    }

    Uint8 mask_high = peek(0x5f31);
    Uint8 mask_low = peek(0x5f32);
    Uint8 solid = 0x00;
    poke(0x5f31, solid);
    poke(0x5f32, solid);

    draw_rect(0, 0, SCREEN_SIZE, SCREEN_SIZE, &color, true);

    poke(0x5f31, mask_high);
    poke(0x5f32, mask_low);

    if (color)
    {
        SDL_SetRenderDrawColor(r, r_prev, g_prev, b_rev, a_prev);
    }

    return 0;
}

static int pico8_color(lua_State* L)
{
    return 0;
}

static int pico8_cursor(lua_State* L)
{
    // Reset position when calling cls().
    return 0;
}

static int pico8_fget(lua_State* L)
{
    return 0;
}

static int pico8_fillp(lua_State* L)
{
    Uint16 pattern = (Uint16)luaL_optunsigned(L, 1, 0);

#if 0
    // Predefined pattern values.
    // P8SCI, 128 - 135.
    switch (pattern)
    {
        case 0:
        case 0x25CB: // 128, Solid.
            pattern = 0x0000;
            break;
        case 0x2588: // 129, Checkerboard.
            pattern = 0x5A5A;
            break;
        case 0x1F431: // 130, Jelpi.
            pattern = 0x511F;
            break;
        case 0x2B07FE0F: // 131, Down key.
            pattern = 0x0003;
            break;
        case 0x2591: // 132, Dot pattern.
            pattern = 0x7070;
            break;
        case 0x273D: // 133, Throwing star.
            pattern = 0x8810;
            break;
        case 0x25CF: // 134, Ball.
            pattern = 0xF99F;
            break;
        case 0x2665: // 135, Heart.
            pattern = 0x51BF;
            break;
        default:
            break;
    }
#endif

    Uint8 high = (pattern & 0xFF00) >> 8;
    Uint8 low = pattern & 0x00FF;

    poke(0x5f31, high);
    poke(0x5f32, low);

    // Set fill pattern mask.
    for (int i = 0; i < 0x4000; i++)
    {
        int row = (i / 128) % 4;
        int col = (i % 128) % 4;

        Uint8 nibble = 0;
        switch (row)
        {
            case 0: nibble = (pattern & 0xF000) >> 12; break;
            case 1: nibble = (pattern & 0x0F00) >> 8;  break;
            case 2: nibble = (pattern & 0x00F0) >> 4;  break;
            case 3: nibble = (pattern & 0x000F);       break;
        }

        Uint8 pixel = (nibble >> (3 - col)) & 0x1;
        fill_mask[i] = pixel ? 0xFF : 0x00;
    }

    return 0;
}

static int pico8_flip(lua_State* L)
{
    return 0;
}

static int pico8_fset(lua_State* L)
{
    return 0;
}

static int pico8_line(lua_State* L)
{
    return 0;
}

static int pico8_oval(lua_State* L)
{
    return 0;
}

static int pico8_ovalfill(lua_State* L)
{
    return 0;
}

static int pico8_pal(lua_State* L)
{
    return 0;
}

static int pico8_palt(lua_State* L)
{
    return 0;
}

static int pico8_pget(lua_State* L)
{
    return 0;
}

static int pico8_print(lua_State* L)
{
    return 0;
}

static int pico8_pset(lua_State* L)
{
    int x = (int)fix32_to_int32(luaL_checknumber(L, 1));
    int y = (int)fix32_to_int32(luaL_checknumber(L, 2));

    if (lua_gettop(L) == 3)
    {
        int color = luaL_checkinteger(L, 3);
        draw_dot(x, y, &color);
    }
    else
    {
        draw_dot(x, y, NULL);
    }

    return 0;
}

static int pico8_rect(lua_State* L)
{
    int x0 = fix32_to_int(luaL_checknumber(L, 1));
    int y0 = fix32_to_int(luaL_checknumber(L, 2));
    int x1 = fix32_to_int(luaL_checknumber(L, 3));
    int y1 = fix32_to_int(luaL_checknumber(L, 4));

    if (lua_gettop(L) == 5)
    {
        int color = fix32_to_int(luaL_checkinteger(L, 5));
        draw_rect(x0, y0, x1, y1, &color, false);
    }
    else
    {
        draw_rect(x0, y0, x1, y1, NULL, false);
    }

    return 0;
}

static int pico8_rectfill(lua_State* L)
{
    int x0 = fix32_to_int(luaL_checknumber(L, 1));
    int y0 = fix32_to_int(luaL_checknumber(L, 2));
    int x1 = fix32_to_int(luaL_checknumber(L, 3));
    int y1 = fix32_to_int(luaL_checknumber(L, 4));

    if (lua_gettop(L) == 5)
    {
        int color = fix32_to_int(luaL_checkinteger(L, 5));
        draw_rect(x0, y0, x1, y1, &color, true);
    }
    else
    {
        draw_rect(x0, y0, x1, y1, NULL, true);
    }

    return 0;
}

static int pico8_sget(lua_State* L)
{
    return 0;
}

static int pico8_spr(lua_State* L)
{
    return 0;
}

static int pico8_sset(lua_State* L)
{
    return 0;
}

static int pico8_sspr(lua_State* L)
{
    return 0;
}

static int pico8_tline(lua_State* L)
{
    return 0;
}

/*******************
 * Math functions. *
 *******************/

// Special thanks to pancelor for documenting rnd() and srand()!
// https://www.lexaloffle.com/bbs/?pid=81103#p
// https://www.lexaloffle.com/bbs/?pid=153638#p

static fix32_t pico8_random(fix32_t limit)
{
    fix32_t result;
    seed_hi = fix32_rotl(seed_hi, 16);
    seed_hi += seed_lo;
    seed_lo += seed_hi;
    if (limit < 0)
    {
        if (limit <= seed_hi && seed_hi < 0)
        {
            return seed_hi - limit;
        }
        else
        {
            return seed_hi;
        }
    }
    else
    {
        if (seed_hi < 0)
        {
            result = fix32_mod(fix32_value(0x7fff, 0), limit) + fix32_value(1, 0);
        }
        else {
            result = 0;
        }
        result += fix32_mod(seed_hi & 0x7fffffff, limit);
        result = fix32_mod(result, limit);
        return result;
    }
}

static int pico8_rnd(lua_State* L)
{
    if (lua_istable(L, 1))
    {
        const fix32_t value = pico8_random(lua_rawlen(L, 1) << 8);
        lua_pushnumber(L, ((value >> 8) + 1) * fix32_value(1, 0));
        lua_gettable(L, -2);
    }
    else
    {
        fix32_t value = fix32_value(1, 0);
        if (lua_gettop(L) > 0)
        {
            value = lua_tonumberx(L, 1, NULL);
        }
        value = pico8_random(value);
        lua_pushnumber(L, value);
    }

    return 1;
}

static int pico8_srand(lua_State* L)
{
    fix32_t new_seed = luaL_checknumber(L, 1);
    if (new_seed == 0)
    {
        seed_hi = 0x60009755;
        seed_lo = 0xdeadbeef;
    }
    else
    {
        new_seed &= 0x7fffffff;
        seed_hi = new_seed ^ 0xbead29ba;
        seed_lo = new_seed;
    }
    for (int i = 0; i < 32; i++)
    {
        pico8_random(0);
    }
    return 0;
}

/*********************
 * Memory functions. *
 *********************/

static int pico8_peek(lua_State* L)
{
  unsigned int addr = luaL_checkunsigned(L, 1);
  unsigned int len = luaL_optunsigned(L, 2, 1);

  if (len > RAM_SIZE - 1)
  {
      len = RAM_SIZE - 1;
  }

  for (unsigned int i = 0; i < len; i++)
  {
      lua_pushnumber(L, (Uint8)peek(addr + i));
  }
  return len;
}

static int pico8_peek2(lua_State* L)
{
  unsigned int addr = luaL_checkunsigned(L, 1);
  unsigned int len = luaL_optunsigned(L, 2, 1);

  if (len > RAM_SIZE - 1)
  {
      len = RAM_SIZE - 1;
  }

  for (unsigned int i = 0; i < len; i++)
  {
      Uint16 data = (Uint16)peek(addr + i) << 8 | (Uint16)peek(addr + i + 1);
      lua_pushnumber(L, data);
  }
  return len;
}

static int pico8_peek4(lua_State* L)
{
  unsigned int addr = luaL_checkunsigned(L, 1);
  unsigned int len = luaL_optunsigned(L, 2, 1);

  if (len > RAM_SIZE - 1)
  {
      len = RAM_SIZE - 1;
  }

  for (unsigned int i = 0; i < len; i++)
  {
      Uint32 data = (Uint32)peek(addr + i) << 24 | (Uint32)peek(addr + i + 1) << 16 | (Uint32)peek(addr + i + 2) << 8 | (Uint32)peek(addr + i + 3);
      lua_pushnumber(L, data);
  }
  return len;
}

static int pico8_poke(lua_State* L)
{
    unsigned int addr = luaL_checkunsigned(L, 1);
    unsigned int n = lua_gettop(L) - 1;

    for (unsigned int i = 0; i < n; i++)
    {
        if (addr + i >= RAM_SIZE)
        {
            break;
        }
        Uint8 data = (Uint8)luaL_checkinteger(L, 2 + i);
        poke(addr + i, data);
    }

    return 0;
}

static int pico8_poke2(lua_State* L)
{
    unsigned int addr = luaL_checkunsigned(L, 1);
    unsigned int n = lua_gettop(L) - 1;
    for (unsigned int i = 0; i < n; i++)
    {
        if (addr + i >= RAM_SIZE - 1)
        {
            break;
        }
        Uint16 data = (Uint16)luaL_checkinteger(L, 2 + i);
        poke(addr + i, (Uint8)(data >> 8));
        poke(addr + i + 1, (Uint8)(data & 0xFF));
    }
    return 0;
}

static int pico8_poke4(lua_State* L)
{
    unsigned int addr = luaL_checkunsigned(L, 1);
    unsigned int n = lua_gettop(L) - 1;
    for (unsigned int i = 0; i < n; i++)
    {
        if (addr + i >= RAM_SIZE - 3)
        {
            break;
        }
        Uint32 data = (Uint32)luaL_checkinteger(L, 2 + i);
        poke(addr + i, (Uint8)(data >> 24));
        poke(addr + i + 1, (Uint8)(data >> 16));
        poke(addr + i + 2, (Uint8)(data >> 8));
        poke(addr + i + 3, (Uint8)(data & 0xFF));
    }
    return 0;
}

/********************
 * Table functions. *
 ********************/

static int pico8_add(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int n = luaL_len(L, 1);
    int index = luaL_optinteger(L, 3, n + 1);
    luaL_argcheck(L, 1 <= index && index <= n + 1, 3, "index out of range");

    for (int i = n; i >= index; i--)
    {
        lua_rawgeti(L, 1, i);
        lua_rawseti(L, 1, i + 1);
    }

    lua_pushvalue(L, 2);
    lua_rawseti(L, 1, index);
    lua_settop(L, 2);
    return 1;
}

static int pico8_foreach(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int n = luaL_len(L, 1);
    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, 1, i);
        lua_pushvalue(L, 2);
        lua_pushvalue(L, -3);
        lua_pushinteger(L, i);
        lua_call(L, 2, 0);
    }
    return 0;
}

/********************
 * Debug functions. *
 ********************/

static int pico8_log(lua_State* L)
{
    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");
    for (int i = 1; i <= n; i++)
    {
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        const char* s = lua_tostring(L, -1);
        if (s == NULL)
        {
            return luaL_error(L, "'tostring' must return a string to 'print'");
        }
        SDL_Log("%s", s);
        lua_pop(L, 1);
    }
    return 0;
}

/*********************
 * API Registration. *
 *********************/

void register_api(lua_State* L, SDL_Renderer* renderer)
{
    r = renderer;
    SDL_memset(&pico8_ram, 0x00, RAM_SIZE);

    // Flow-control.
    lua_pushcfunction(L, pico8_time);
    lua_setglobal(L, "time");
    lua_pushcfunction(L, pico8_time);
    lua_setglobal(L, "t");

    // Graphics.
    lua_pushcfunction(L, pico8_camera);
    lua_setglobal(L, "camera");
    lua_pushcfunction(L, pico8_circ);
    lua_setglobal(L, "circ");
    lua_pushcfunction(L, pico8_circfill);
    lua_setglobal(L, "circfill");
    lua_pushcfunction(L, pico8_clip);
    lua_setglobal(L, "clip");
    lua_pushcfunction(L, pico8_cls);
    lua_setglobal(L, "cls");
    lua_pushcfunction(L, pico8_color);
    lua_setglobal(L, "color");
    lua_pushcfunction(L, pico8_cursor);
    lua_setglobal(L, "cursor");
    lua_pushcfunction(L, pico8_fget);
    lua_setglobal(L, "fget");
    lua_pushcfunction(L, pico8_fillp);
    lua_setglobal(L, "fillp");
    lua_pushcfunction(L, pico8_flip);
    lua_setglobal(L, "flip");
    lua_pushcfunction(L, pico8_fset);
    lua_setglobal(L, "fset");
    lua_pushcfunction(L, pico8_line);
    lua_setglobal(L, "line");
    lua_pushcfunction(L, pico8_oval);
    lua_setglobal(L, "oval");
    lua_pushcfunction(L, pico8_ovalfill);
    lua_setglobal(L, "ovalfill");
    lua_pushcfunction(L, pico8_pal);
    lua_setglobal(L, "pal");
    lua_pushcfunction(L, pico8_palt);
    lua_setglobal(L, "palt");
    lua_pushcfunction(L, pico8_pget);
    lua_setglobal(L, "pget");
    lua_pushcfunction(L, pico8_print);
    lua_setglobal(L, "print");
    lua_pushcfunction(L, pico8_pset);
    lua_setglobal(L, "pset");
    lua_pushcfunction(L, pico8_rect);
    lua_setglobal(L, "rect");
    lua_pushcfunction(L, pico8_rectfill);
    lua_setglobal(L, "rectfill");
    lua_pushcfunction(L, pico8_sget);
    lua_setglobal(L, "sget");
    lua_pushcfunction(L, pico8_spr);
    lua_setglobal(L, "spr");
    lua_pushcfunction(L, pico8_sset);
    lua_setglobal(L, "sset");
    lua_pushcfunction(L, pico8_sspr);
    lua_setglobal(L, "sspr");
    lua_pushcfunction(L, pico8_tline);
    lua_setglobal(L, "tline");

    // Math.
    static bool seed_initialized = false;
    if (!seed_initialized)
    {
        seed_lo = (fix32_t)SDL_GetPerformanceCounter();
        SDL_DelayNS(SDL_NS_PER_US);
        seed_hi = (fix32_t)SDL_GetPerformanceCounter();
        seed_initialized = true;
    }

    lua_pushcfunction(L, pico8_rnd);
    lua_setglobal(L, "rnd");
    lua_pushcfunction(L, pico8_srand);
    lua_setglobal(L, "srand");

    // Memory.
    lua_pushcfunction(L, pico8_peek);
    lua_setglobal(L, "peek");
    lua_pushcfunction(L, pico8_peek2);
    lua_setglobal(L, "peek2");
    lua_pushcfunction(L, pico8_peek4);
    lua_setglobal(L, "peek4");
    lua_pushcfunction(L, pico8_poke);
    lua_setglobal(L, "poke");
    lua_pushcfunction(L, pico8_poke2);
    lua_setglobal(L, "poke2");
    lua_pushcfunction(L, pico8_poke4);
    lua_setglobal(L, "poke4");

    // Tables.
    lua_pushcfunction(L, pico8_add);
    lua_setglobal(L, "add");
    lua_pushcfunction(L, pico8_foreach);
    lua_setglobal(L, "foreach");

    // Debug.
    lua_pushcfunction(L, pico8_log);
    lua_setglobal(L, "log");
}

void reset_draw_state(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_memset(&pico8_ram, 0x00, RAM_SIZE);
}

void update_time(void)
{
    seconds_since_start = (SDL_GetPerformanceCounter() * 3435973837ULL) >> 35;
}
