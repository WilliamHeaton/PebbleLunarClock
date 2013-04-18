#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <math.h>
#include <float.h>


#define MY_UUID { 0x01, 0x15, 0x7D, 0x68, 0xA7, 0xE3, 0x4F, 0x87, 0xAB, 0x2C, 0xBB, 0x79, 0x9F, 0x3E, 0x92, 0x9D }
PBL_APP_INFO(MY_UUID,
             "Lunar Clock", "William Heaton",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

Layer moon_layer;
Layer shadow_layer;

int radius  = 70;
int border = 1;
int centerx = 71;
int centery = 167-70;

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

void moon_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, GPoint(centerx,centery), radius);
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
}

void day_tick(AppContextRef ctx, PebbleTickEvent *t) {
    (void)ctx;
    layer_mark_dirty(&shadow_layer);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
        .tick_handler = &day_tick,
        .tick_units = DAY_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
