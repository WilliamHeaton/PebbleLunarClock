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

Layer moon_layer;
Layer shadow_layer;
Layer phase_layer;
Layer second_layer;

int radius  = 65;
int border = 1;
int centerx = 71;
int centery = 167-65;

const char phases[8][20] = {"New Moon","Waxing Crescent","First Quarter","Waxing Gibbous","Full Moon","Waning Gibbous","Thrid Quarter","Waning Crescent"};


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
int daysSinceJan_1_1900( int year, int yday){
     
    int delta = yday;
    for(int i = 1900; i<year; i++){
        delta += 365;
        if(leapYear(i))
            delta += 1;            
    }
    return delta;
}

double phase(){
    PblTm currentTime;
    get_time(&currentTime);
    double delta = daysSinceJan_1_1900(currentTime.tm_year+1900,currentTime.tm_yday);
    double p = 29.530588853;
    return delta - p * floor(delta / p);
}

double phasePercent(){
    return phase()/29.530588853;
}

char* intToStr(int val){
 	static char buf[32] = {0};
	int i = 30;	
	for(; val && i ; --i, val /= 10)
		buf[i] = "0123456789"[val % 10];
	
	return &buf[i+1];
}



void second_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    PblTm currentTime;
    get_time(&currentTime);
    
    int cols[8][2] = 
                        {{GColorWhite,GColorWhite},
                        {GColorBlack,GColorWhite},
                        {GColorBlack,GColorWhite},
                        {GColorBlack,GColorWhite},
                        {GColorBlack,GColorBlack},
                        {GColorWhite,GColorBlack},
                        {GColorWhite,GColorBlack},
                        {GColorWhite,GColorBlack}};
    
    int p = floor(16.*(phasePercent()+0.0625));
    p = (p % 16)/2;
        
    int ox = -1;
    int oy = 1;
    int c1 = cols[p][0];
    int c2 = cols[p][1];
    
    if(currentTime.tm_sec<15 || currentTime.tm_sec>45){
        oy = -1;
    }if(currentTime.tm_sec>30){
        ox = 1;
        c1 = cols[p][1];
        c2 = cols[p][0];
    }
    
    double c = cos_lookup(TRIG_MAX_ANGLE/-4.+TRIG_MAX_ANGLE * ((double)currentTime.tm_sec)/60.) / 65536.;
    double s = sin_lookup(TRIG_MAX_ANGLE/-4.+TRIG_MAX_ANGLE * ((double)currentTime.tm_sec)/60.) / 65536.;
    
    if(c1!=c2){
        graphics_context_set_stroke_color(ctx, c2);
        
        graphics_draw_line(
            ctx, 
            GPoint(centerx+ox, centery+oy), 
            GPoint(
                centerx + (c*radius*0.96)+ox, 
                centery + (s*radius*0.96)+oy));
    }
    
    graphics_context_set_stroke_color(ctx, c1);
    graphics_draw_line(
        ctx, 
        GPoint(centerx, centery), 
        GPoint(
            centerx + (c*radius*0.96), 
            centery + (s*radius*0.96)));
    
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, GPoint(centerx + (c*radius*0.9),centery + (s*radius*0.9)), 3);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, GPoint(centerx + (c*radius*0.9),centery + (s*radius*0.9)), 2);
    
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_text_draw(
        ctx, 
        intToStr(currentTime.tm_sec),  
        fonts_get_system_font(FONT_KEY_GOTHIC_24), 
        GRect(0, 0, 30, 30), 
        GTextOverflowModeWordWrap, 
        GTextAlignmentCenter, 
        NULL);
}

void moon_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, GPoint(centerx,centery), radius);
}
void phase_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    int p = floor(16.*(phasePercent()+0.0625));
    p = (p % 16)/2;
        
    graphics_text_draw(
        ctx, 
        phases[p],  
        fonts_get_system_font(FONT_KEY_GOTHIC_24), 
        GRect(0, 0, 144, 30), 
        GTextOverflowModeWordWrap, 
        GTextAlignmentCenter, 
        NULL);
    
}
void shadow_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorBlack);
    
    double p = phasePercent();    
    
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
    
    while(x < y){
        if(f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;  
        
        graphics_draw_line(ctx, GPoint(centerx + off1*x, centery + y), GPoint(centerx + off2*x, centery + y));
        graphics_draw_line(ctx, GPoint(centerx + off1*x, centery - y), GPoint(centerx + off2*x, centery - y));
        graphics_draw_line(ctx, GPoint(centerx + off1*y, centery + x), GPoint(centerx + off2*y, centery + x));
        graphics_draw_line(ctx, GPoint(centerx + off1*y, centery - x), GPoint(centerx + off2*y, centery - x));
    }
    graphics_draw_line(ctx, GPoint(centerx + off1*r, centery), GPoint((centerx + off2*r), centery));
}



void handle_init(AppContextRef ctx) {
    (void)ctx;

    window_init(&window, "Watchface");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);
    
    layer_init(&moon_layer, window.layer.frame);
    moon_layer.update_proc = &moon_layer_update_callback;
    layer_add_child(&window.layer, &moon_layer);
    
    layer_init(&shadow_layer, window.layer.frame);
    shadow_layer.update_proc = &shadow_layer_update_callback;
    layer_add_child(&window.layer, &shadow_layer);
    
    layer_init(&phase_layer, window.layer.frame);
    phase_layer.update_proc = &phase_layer_update_callback;
    layer_add_child(&window.layer, &phase_layer);
    
    layer_init(&second_layer, window.layer.frame);
    second_layer.update_proc = &second_layer_update_callback;
    layer_add_child(&window.layer, &second_layer);
}

void second_tick(AppContextRef ctx, PebbleTickEvent *t) {
    (void)ctx;
    
    if (t->units_changed & DAY_UNIT) {
        layer_mark_dirty(&shadow_layer);
        layer_mark_dirty(&phase_layer);
    }
    if (t->units_changed & SECOND_UNIT) {
        layer_mark_dirty(&second_layer);
    }

    
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
        .tick_handler = &second_tick,
        .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
