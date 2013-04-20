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

bool showSeconds = true;
int timezone = -5;

int radius  = 68;
int border = 1;
int centerx = 71;
int centery = 71;

double hourLen = 0.60;
double minLen  = 0.75;
double secLen  = 0.88;
int    secRad  = 3;

const char phases[8][20] = {"New Moon","Waxing Crescent","First Quarter","Waxing Gibbous","Full Moon","Waning Gibbous","Third Quarter","Waning Crescent"};

int ph;
double phase;
double phasePercent;
int curHour;
int curMin;
int curSec;

Layer moon_layer;
Layer shadow_layer;
Layer phase_layer;
Layer second_layer;
Layer minute_layer;
Layer hour_layer;


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

char* intToStr(int val){
 	static char buf[32] = {0};
	int i = 30;	
	for(; val && i ; --i, val /= 10)
		buf[i] = "0123456789"[val % 10];
	
	return &buf[i+1];
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
                    
void drawHand(Layer *me, GContext* ctx,double sec,int r,bool endCirc,bool startCirc){
    
    int c1 = cols[ph][sec>30?1:0];
    int c2 = cols[ph][sec>30?0:1];
    int c3 = (c1==c2)?cols[ph][2]:c2;
    
    int ox = -1;
    int oy = 1;
    if(sec>30){
        ox = 1;
    }if(sec<15 || sec>45){
        oy = -1;
    }
    
    double angle = TRIG_MAX_ANGLE/-4.+TRIG_MAX_ANGLE * sec/60.;
    double c = cos_lookup(angle) / 65536.;
    double s = sin_lookup(angle) / 65536.;
    
    graphics_context_set_stroke_color(ctx, c3);
    graphics_draw_line(
        ctx, 
        GPoint(centerx+ox, centery+oy), 
        GPoint(
            centerx + c*r+ox, 
            centery + s*r+oy));
    
    graphics_context_set_stroke_color(ctx, c1);
    graphics_draw_line(
        ctx, 
        GPoint(centerx, centery), 
        GPoint(
            centerx + c*r, 
            centery + s*r));
    
    if(startCirc){
        graphics_context_set_fill_color(ctx, c3);
        graphics_fill_circle(ctx, GPoint(centerx,centery), 5);
        graphics_context_set_fill_color(ctx, c1);
        graphics_fill_circle(ctx, GPoint(centerx,centery), 2);
    }
    if(endCirc){
        double cc = r+secRad;
        graphics_context_set_fill_color(ctx, cols2[ph][0]);
        graphics_fill_circle(ctx, GPoint(centerx + c*cc,centery + s*cc), secRad);
        graphics_context_set_fill_color(ctx, cols2[ph][1]);
        graphics_fill_circle(ctx, GPoint(centerx + c*cc,centery + s*cc), secRad-1);
    }
}
void second_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    drawHand(me, ctx, curSec, radius*secLen,true,true);
}

void minute_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    drawHand(me, ctx, curMin, radius*minLen,false,false);
}
void hour_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    drawHand(me, ctx, ((curHour%12)+curMin/60.)*60./12., radius*hourLen,false,false);
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
    
    PblTm t;
    get_time(&t);
    double delta = daysSinceNewMoon(t.tm_year+1900,t.tm_yday,t.tm_hour);
    phase = delta - 29.530588853 * floor(delta / 29.530588853);
    phasePercent = phase/29.530588853;
    ph = ((int)floor(16.*(phasePercent+0.09)) % 16)/2;
    
    curHour=t.tm_hour;
    curMin=t.tm_min;
    curSec=t.tm_sec;
    
    layer_init(&moon_layer, window.layer.frame);
    moon_layer.update_proc = &moon_layer_update_callback;
    layer_add_child(&window.layer, &moon_layer);
    
    layer_init(&shadow_layer, window.layer.frame);
    shadow_layer.update_proc = &shadow_layer_update_callback;
    layer_add_child(&window.layer, &shadow_layer);
    
    layer_init(&phase_layer, window.layer.frame);
    phase_layer.update_proc = &phase_layer_update_callback;
    layer_add_child(&window.layer, &phase_layer);
    
    layer_init(&hour_layer, window.layer.frame);
    hour_layer.update_proc = &hour_layer_update_callback;
    layer_add_child(&window.layer, &hour_layer);
    
    layer_init(&minute_layer, window.layer.frame);
    minute_layer.update_proc = &minute_layer_update_callback;
    layer_add_child(&window.layer, &minute_layer);
    
    if(showSeconds){
        layer_init(&second_layer, window.layer.frame);
        second_layer.update_proc = &second_layer_update_callback;
        layer_add_child(&window.layer, &second_layer);
    }
    
    
}

void second_tick(AppContextRef ctx, PebbleTickEvent *t) {
    (void)ctx;
    
    curHour=t->tick_time->tm_hour;
    curMin=t->tick_time->tm_min;
    curSec=t->tick_time->tm_sec;

    if (showSeconds && (t->units_changed & SECOND_UNIT)) {
        layer_mark_dirty(&second_layer);
    }
    if (t->units_changed & MINUTE_UNIT) {
        layer_mark_dirty(&minute_layer);
        layer_mark_dirty(&hour_layer);
    }
    if (t->units_changed & HOUR_UNIT) {
    
        double delta = daysSinceNewMoon(t->tick_time->tm_year+1900,t->tick_time->tm_yday,t->tick_time->tm_hour);
        phase = delta - 29.530588853 * floor(delta / 29.530588853);
        phasePercent = phase/29.530588853;
        ph = ((int)floor(16.*(phasePercent+0.09)) % 16)/2;
        
        layer_mark_dirty(&shadow_layer);
        layer_mark_dirty(&phase_layer);
    }
}

void pbl_main(void *params) {
    

  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
        .tick_handler = &second_tick,
        .tick_units = showSeconds?SECOND_UNIT:MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
