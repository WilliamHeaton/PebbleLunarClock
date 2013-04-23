#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <math.h>
#include "resource_ids.auto.h"


#define MY_UUID { 0x01, 0x15, 0x7D, 0x68, 0xA7, 0xE3, 0x4F, 0x87, 0xAB, 0x2C, 0xBB, 0x79, 0x9F, 0x3E, 0x92, 0x9D }
PBL_APP_INFO(MY_UUID,
             "Lunar Phase", "William Heaton",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

const bool showSeconds = true;
const bool showMinutes = true;
const bool showHours   = true; 
const bool showDetailedMoonGraphic = true;
const bool showAnimation = true;

const int animationSpeed = 35;
const double animationStep = 0.08;

const int timezone = -5;

const int radius  = 68;
const int border = 1;
const int centerx = 71;
const int centery = 71;

const double secLen  = 0.94;
const int    secRad  = 3;

const char phases[8][20] = {"New Moon","Waxing Crescent","First Quarter","Waxing Gibbous","Full Moon","Waning Gibbous","Third Quarter","Waning Crescent"};

int ph;
const double phaseLength = 29.530588853;
double phase;
double phasePercent;
int curHour;
int curMin;
int curSec;
int animationCount = 0;

Layer moon_layer;
Layer shadow_layer;
Layer phase_layer;
Layer second_layer;
Layer minute_layer;
Layer hour_layer;
Layer top_layer;
BmpContainer moonimage_container;

AppTimerHandle timer_handle;

bool leapYear(int year){
    if(year%400==0)
        return true;
    else if(year%100==0)
        return false;
    else if(year%4==0)
        return true;
    else 
        return false;
}
double daysSinceNewMoon( int year, int yday, int hour){
    
    double delta = -14.01+yday;
    for(int i = 2010; i<year; i++){
        delta += 365;
        if(leapYear(i))
            delta += 1;            
    }
    return delta+(hour+5+timezone)/24.;
}

const int cols[8][3] = 
                    {{GColorWhite,GColorWhite,GColorBlack},
                    {GColorBlack,GColorWhite},
                    {GColorBlack,GColorWhite},
                    {GColorBlack,GColorWhite},
                    {GColorBlack,GColorBlack,GColorWhite},
                    {GColorWhite,GColorBlack},
                    {GColorWhite,GColorBlack},
                    {GColorWhite,GColorBlack}};

const int cols2[8][2] = 
                    {{GColorWhite,GColorBlack}, // NEW MOON
                    {GColorWhite,GColorBlack},
                    {GColorBlack,GColorWhite}, // Quarter
                    {GColorBlack,GColorWhite},
                    {GColorBlack,GColorWhite}, // FULL MOON
                    {GColorBlack,GColorWhite},
                    {GColorWhite,GColorBlack}, // Quarter
                    {GColorWhite,GColorBlack}};
                    

GPath hour_hand;
const GPathInfo hour_hand_info = {
    5,
    (GPoint []) {
    {-2, -7},
    { 2, -7},
    { 7, -12},
    { 0, -52},
    {-7, -12},
    }
};

GPath minute_hand;
const GPathInfo minute_hand_info = {
    6,
    (GPoint []) {
    {-4, -8},
    { 0, -7},
    { 4, -8},
    { 4, -30},
    { 0, -65},
    {-4, -30},
    }
};
void top_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    graphics_context_set_fill_color(ctx, cols2[ph][1]);
    graphics_fill_circle(ctx, GPoint(centerx,centery), 5);
    graphics_context_set_fill_color(ctx, cols2[ph][0]);
    graphics_fill_circle(ctx, GPoint(centerx,centery), 2);

}
void second_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    int r = radius*secLen;
    
    int c1 = cols[ph][curSec>30?1:0];
    int c2 = cols[ph][curSec>30?0:1];
    int c3 = (c1==c2)?cols[ph][2]:c2;
    
    int ox = -1;
    int oy = 1;
    if(curSec>30){
        ox = 1;
    }if(curSec<15 || curSec>45){
        oy = -1;
    }
    
    int angle = TRIG_MAX_ANGLE/-4+TRIG_MAX_ANGLE * curSec/60.;
    double x = cos_lookup(angle) / 65536. * r;
    double y = sin_lookup(angle) / 65536. * r;
    
    graphics_context_set_stroke_color(ctx, c3);
    graphics_draw_line(
        ctx, 
        GPoint(centerx+ox, centery+oy), 
        GPoint(
            centerx + x + ox, 
            centery + y + oy));
    
    graphics_context_set_stroke_color(ctx, c1);
    graphics_draw_line(
        ctx, 
        GPoint(centerx, centery), 
        GPoint(
            centerx + x, 
            centery + y));
    
    graphics_context_set_fill_color(ctx, showDetailedMoonGraphic?GColorWhite:c1);
    graphics_fill_circle(ctx, GPoint(centerx + x,centery + y), secRad);
    
    graphics_context_set_fill_color(ctx, showDetailedMoonGraphic?GColorBlack:c2);
    graphics_fill_circle(ctx, GPoint(centerx + x,centery + y), secRad-1);
}

void minute_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    gpath_rotate_to(&minute_hand, (TRIG_MAX_ANGLE / 360) * curMin * 6);

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorBlack);
    gpath_draw_filled(ctx, &minute_hand);
    gpath_draw_outline(ctx, &minute_hand);

}
void hour_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    gpath_rotate_to(&hour_hand, (TRIG_MAX_ANGLE / 360) * (curHour * 30 + curMin / 2));

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorBlack);
    gpath_draw_filled(ctx, &hour_hand);
    gpath_draw_outline(ctx, &hour_hand);

}
void moon_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, GPoint(centerx,centery), radius);
}
void phase_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_text_draw(
        ctx, 
        phases[ph],  
        fonts_get_system_font(FONT_KEY_GOTHIC_24), 
        GRect(0, 138, 144, 30), 
        GTextOverflowModeWordWrap, 
        GTextAlignmentCenter, 
        NULL);
    
}
void shadow_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorBlack);
    
    double p = phasePercent;
    
    double off2;
    int off1 = -1;
    if(p < 0.5)
        off2 = 1.-4.*p;
    else {
        off2 = -(4.*p-3.);
        off1 = 1;
    }
    
    int r = radius - border;
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    int ox1,ox2,oy1,oy2;
    while(x < y){
        if(f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;  
        ox1 = off1*x;
        oy1 = off1*y;
        ox2 = off2*x;
        oy2 = off2*y;
        graphics_draw_line(ctx, GPoint(centerx + ox1, centery + y), GPoint(centerx + ox2, centery + y));
        graphics_draw_line(ctx, GPoint(centerx + ox1, centery - y), GPoint(centerx + ox2, centery - y));
        graphics_draw_line(ctx, GPoint(centerx + oy1, centery + x), GPoint(centerx + oy2, centery + x));
        graphics_draw_line(ctx, GPoint(centerx + oy1, centery - x), GPoint(centerx + oy2, centery - x));
    }
    graphics_draw_line(ctx, GPoint(centerx + off1*r, centery), GPoint((centerx + off2*r), centery));
}

void setPhase(double delta){
    phase = delta - phaseLength * floor(delta / phaseLength);
    phasePercent = phase/phaseLength;
    
    double p = phasePercent * 200;
    double d = 100/phaseLength;
    
    if(     p<   0.+d 
         || p> 200.-d)
        ph = 0;
    else if(p<= 50.-d)
        ph = 1;
    else if(p<  50.+d)
        ph = 2;
    else if(p<=100.-d)
        ph = 3;
    else if(p< 100.+d)
        ph = 4;
    else if(p<=150.-d)
        ph = 5;
    else if(p< 150.+d)
        ph = 6;
    else
        ph = 7;
}
void animate_moon(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
    (void)ctx;
    (void)handle;
    animationCount++;
    
    if(animationCount*animationStep <= 1 ){
        phasePercent += animationStep;
        if(phasePercent>=1)phasePercent--;
        layer_mark_dirty(&shadow_layer);
        timer_handle = app_timer_send_event(ctx, animationSpeed, 1);
    }
    else{
        PblTm t;
        get_time(&t);
        setPhase(daysSinceNewMoon(t.tm_year+1900,t.tm_yday,t.tm_hour));
        
        curHour=t.tm_hour;
        curMin=t.tm_min;
        curSec=t.tm_sec;
        layer_mark_dirty(&shadow_layer);
        layer_mark_dirty(&phase_layer);
        if(showHours){
            layer_mark_dirty(&hour_layer);
            layer_mark_dirty(&minute_layer);
            layer_mark_dirty(&top_layer);
        }
    }
}


void handle_deinit(AppContextRef ctx) {
  (void)ctx;
  bmp_deinit_container(&moonimage_container);
}
void handle_init(AppContextRef ctx) {
    (void)ctx;

    window_init(&window, "Watchface");
    window_stack_push(&window, false /* Animated */);
    window_set_background_color(&window, GColorBlack);
        
    PblTm t;
    get_time(&t);
    setPhase(daysSinceNewMoon(t.tm_year+1900,t.tm_yday,t.tm_hour));
    
    curHour=t.tm_hour;
    curMin=t.tm_min;
    curSec=t.tm_sec;
    
    if(showDetailedMoonGraphic){
        
        resource_init_current_app(&LUNARCLOCK_IMAGE_RESOURCES);
        bmp_init_container(RESOURCE_ID_IMAGE_MOON, &moonimage_container);
        layer_add_child(&window.layer, &moonimage_container.layer.layer);
    }else{
        layer_init(&moon_layer, window.layer.frame);
        moon_layer.update_proc = &moon_layer_update_callback;
        layer_add_child(&window.layer, &moon_layer);
    }
    layer_init(&shadow_layer, window.layer.frame);
    shadow_layer.update_proc = &shadow_layer_update_callback;
    layer_add_child(&window.layer, &shadow_layer);
    
    layer_init(&phase_layer, window.layer.frame);
    phase_layer.update_proc = &phase_layer_update_callback;
    layer_add_child(&window.layer, &phase_layer);
    
    if(showHours){
        layer_init(&hour_layer, window.layer.frame);
        hour_layer.update_proc = &hour_layer_update_callback;
        layer_add_child(&window.layer, &hour_layer);
        
        gpath_init(&hour_hand, &hour_hand_info);
        gpath_move_to(&hour_hand, GPoint(centerx, centery));
        
    }
    if(showMinutes){
        layer_init(&minute_layer, window.layer.frame);
        minute_layer.update_proc = &minute_layer_update_callback;
        layer_add_child(&window.layer, &minute_layer);
        
        gpath_init(&minute_hand, &minute_hand_info);
        gpath_move_to(&minute_hand, GPoint(centerx, centery));
    }
    if(showSeconds){
        layer_init(&second_layer, window.layer.frame);
        second_layer.update_proc = &second_layer_update_callback;
        layer_add_child(&window.layer, &second_layer);
    }
    if(showHours){
        layer_init(&top_layer, window.layer.frame);
        top_layer.update_proc = &top_layer_update_callback;
        layer_add_child(&window.layer, &top_layer);
    }
    if(showAnimation)
        timer_handle = app_timer_send_event(ctx, animationSpeed, 1);
}

void second_tick(AppContextRef ctx, PebbleTickEvent *t) {
    (void)ctx;
    
    curHour=t->tick_time->tm_hour;
    curMin=t->tick_time->tm_min;
    curSec=t->tick_time->tm_sec;

    if (showSeconds && (t->units_changed & SECOND_UNIT)) {
        layer_mark_dirty(&second_layer);
    }
    if (showMinutes && t->units_changed & MINUTE_UNIT) {
        layer_mark_dirty(&minute_layer);
        layer_mark_dirty(&hour_layer);
    }
    if (t->units_changed & HOUR_UNIT) {
        
        setPhase(daysSinceNewMoon(t->tick_time->tm_year+1900,t->tick_time->tm_yday,t->tick_time->tm_hour));
        
        layer_mark_dirty(&shadow_layer);
        layer_mark_dirty(&phase_layer);
        
        if(showHours){
            layer_mark_dirty(&top_layer);
        }
    }
}

void pbl_main(void *params) {

    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,
        .timer_handler = &animate_moon,
        .tick_info = {
            .tick_handler = &second_tick,
            .tick_units = showSeconds?SECOND_UNIT:MINUTE_UNIT
        }
    };
    app_event_loop(params, &handlers);
}
