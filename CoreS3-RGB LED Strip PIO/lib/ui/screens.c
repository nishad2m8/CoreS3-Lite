#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            static const lv_img_dsc_t *anim_imgs[20] = {
                &img_frame_00,
                &img_frame_01,
                &img_frame_02,
                &img_frame_03,
                &img_frame_04,
                &img_frame_05,
                &img_frame_06,
                &img_frame_07,
                &img_frame_08,
                &img_frame_09,
                &img_frame_10,
                &img_frame_11,
                &img_frame_12,
                &img_frame_13,
                &img_frame_14,
                &img_frame_15,
                &img_frame_16,
                &img_frame_17,
                &img_frame_18,
                &img_frame_19,
            };

            lv_obj_t *animimg = lv_animimg_create(parent_obj);
            objects.anim_tree = animimg;
            lv_animimg_set_src(animimg, (const void **)anim_imgs, 20);
            lv_animimg_set_duration(animimg, 1000);
            lv_animimg_set_repeat_count(animimg, LV_ANIM_REPEAT_INFINITE);
            lv_obj_center(animimg);
            lv_animimg_start(animimg);
        }
        {
            // slider volume (invisible, left side)
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.slider_volume = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 75, 240);
            lv_slider_set_range(obj, 0, 255);
            lv_slider_set_value(obj, 100, LV_ANIM_OFF);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
        }
        {
            // button mode (invisible, right side)
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.button_mode = obj;
            lv_obj_set_pos(obj, 220, 0);
            lv_obj_set_size(obj, 100, 240);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_shadow_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    tick_screen_main();
}

void tick_screen_main() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
}
