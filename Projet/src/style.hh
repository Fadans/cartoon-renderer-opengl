#pragma once

struct Style {
    const char* name;
    float num_bands;
    bool  hatch;
    float ambient, spec_thr, spec_str;
    float outline[3], outline_far[3];
    float thickness, thickness_far;
    float bg_top[3], bg_bottom[3];
};

static const Style g_styles[] = {
    { "Croquis encre",   3.f,  true,  .28f, .70f, .60f, {.10f,.07f,.05f}, {.10f,.07f,.05f}, 2.5f, 1.0f, {.90f,.86f,.76f}, {.80f,.74f,.62f} },
    { "Pop comics",      5.f,  false, .22f, .50f, .90f, {0,0,0},          {0,0,0},          7.f, 1.0f, {.25f,.62f,.96f}, {.99f,.85f,.38f} },
    { "Phong (compar.)", 64.f, false, .12f, .40f, .85f, {0,0,0},          {0,0,0},          0.f, 1.0f, {.80f,.82f,.86f}, {.60f,.62f,.66f} },
};
static const int g_style_count = (int)(sizeof(g_styles) / sizeof(g_styles[0]));
