// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.5.0
// LVGL version: 8.3.11
// Project name: C5_Dual_antenna

#include "../ui.h"

void ui_ScreenM_screen_init(void)
{
    ui_ScreenM = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ScreenM, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_ScreenM, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ScreenM, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ScreenM_LabelPoint = lv_label_create(ui_ScreenM);
    lv_obj_set_width(ui_ScreenM_LabelPoint, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ScreenM_LabelPoint, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_ScreenM_LabelPoint, 0);
    lv_obj_set_y(ui_ScreenM_LabelPoint, 97);
    lv_obj_set_align(ui_ScreenM_LabelPoint, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ScreenM_LabelPoint, "....");
    lv_obj_set_style_text_color(ui_ScreenM_LabelPoint, lv_color_hex(0xBEBEBE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ScreenM_LabelPoint, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_ScreenM_LabelPoint, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ScreenM_LabelPointSelect = lv_label_create(ui_ScreenM);
    lv_obj_set_width(ui_ScreenM_LabelPointSelect, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ScreenM_LabelPointSelect, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_ScreenM_LabelPointSelect, 5);
    lv_obj_set_y(ui_ScreenM_LabelPointSelect, 97);
    lv_obj_set_align(ui_ScreenM_LabelPointSelect, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ScreenM_LabelPointSelect, ".");
    lv_obj_set_style_text_font(ui_ScreenM_LabelPointSelect, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ScreenM_Button1 = lv_btn_create(ui_ScreenM);
    lv_obj_set_width(ui_ScreenM_Button1, 100);
    lv_obj_set_height(ui_ScreenM_Button1, 50);
    lv_obj_set_x(ui_ScreenM_Button1, 0);
    lv_obj_set_y(ui_ScreenM_Button1, -55);
    lv_obj_set_align(ui_ScreenM_Button1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_ScreenM_Button1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_ScreenM_Button1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_ScreenM_Button1, lv_color_hex(0xDAFFB2), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ScreenM_Button1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_ScreenM_Button1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_ScreenM_Button1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_ScreenM_Button1, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_ScreenM_Button1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_ScreenM_Button1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(ui_ScreenM_Button1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(ui_ScreenM_Button1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ScreenM_Label6 = lv_label_create(ui_ScreenM_Button1);
    lv_obj_set_width(ui_ScreenM_Label6, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ScreenM_Label6, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_ScreenM_Label6, LV_ALIGN_CENTER);
    lv_obj_set_style_text_color(ui_ScreenM_Label6, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ScreenM_Label6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ScreenM_Image1 = lv_img_create(ui_ScreenM);
    lv_img_set_src(ui_ScreenM_Image1, &ui_img_272184077);
    lv_obj_set_width(ui_ScreenM_Image1, LV_SIZE_CONTENT);   /// 200
    lv_obj_set_height(ui_ScreenM_Image1, LV_SIZE_CONTENT);    /// 200
    lv_obj_set_align(ui_ScreenM_Image1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_ScreenM_Image1, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_ScreenM_Image1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_angle(ui_ScreenM_Image1, 900);

    ui_ScreenM_Panel1 = lv_obj_create(ui_ScreenM);
    lv_obj_set_width(ui_ScreenM_Panel1, 70);
    lv_obj_set_height(ui_ScreenM_Panel1, 70);
    lv_obj_set_align(ui_ScreenM_Panel1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_ScreenM_Panel1, LV_OBJ_FLAG_HIDDEN);     /// Flags
    lv_obj_clear_flag(ui_ScreenM_Panel1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_ScreenM_Panel1, 100, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ScreenM_Label1 = lv_label_create(ui_ScreenM_Panel1);
    lv_obj_set_width(ui_ScreenM_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ScreenM_Label1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_ScreenM_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ScreenM_Label1, "360.0°");
    lv_obj_set_style_text_font(ui_ScreenM_Label1, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_ScreenM_Panel1, ui_event_ScreenM_Panel1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ScreenM, ui_event_ScreenM, LV_EVENT_ALL, NULL);

}
