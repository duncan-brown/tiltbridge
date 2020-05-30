//
// Created by John Beeler on 5/12/18.
//

#include "bridge_lcd.h"
#include "img/fermentrack_logo.h"

bridge_lcd lcd;


#include <Wire.h>
#include <TFT_eSPI.h> 
#include <SPI.h>




bridge_lcd::bridge_lcd() {
    next_screen_at = 0;
    on_screen = 0;  // Initialize to 0 (AKA screen_tilt)
    tilt_on_page = 0;
    tilt_pages_in_run = 0;

}  // bridge_lcd




void bridge_lcd::display_logo() {
    // XBM files are C source bitmap arrays, and can be created in GIMP (and then read/imported using text editors)
    clear();
    oled_display->drawXBitmap((oled_display->width()-fermentrack_logo_width)/2, (oled_display->height()-fermentrack_logo_height)/2, fermentrack_logo_bits, fermentrack_logo_width, fermentrack_logo_height, TFT_WHITE);
    display();
}


void bridge_lcd::check_screen() {
    if(next_screen_at < xTaskGetTickCount()) {
        next_screen_at = display_next() * 1000 + xTaskGetTickCount();
    }
}

// display_next returns the number of seconds to "hold" on this screen
uint8_t bridge_lcd::display_next() {
    uint8_t active_tilts = 0;

    if(on_screen==SCREEN_TILT) {
        if(tilt_pages_in_run==0) {
            // This is the first time we're displaying a tilt screen in this round. Figure out how many pages we need
            for(uint8_t i = 0;i<TILT_COLORS;i++) {
                if (tilt_scanner.tilt(i)->is_loaded())
                    active_tilts++;
            }

            // We'll always have at least one page, but we can have more
            tilt_pages_in_run = (active_tilts/TILTS_PER_PAGE) + 1;
            tilt_on_page = 0;
        }

        display_tilt_screen(tilt_on_page);

        tilt_on_page++;
        if(tilt_on_page >= tilt_pages_in_run) {
            tilt_pages_in_run = 0;  // We've displayed the last page
            tilt_on_page = 0;
            on_screen++;
        }

        return 10;  // Display this screen for 10 seconds

    } else if(on_screen==SCREEN_FERMENTRACK) {
        display_logo();
        on_screen++;
        return 5;  // This is currently a noop
    } else {
        on_screen = SCREEN_TILT;
        return 0; // Immediately move on to the next screen
    }

}


void bridge_lcd::display_tilt_screen(uint8_t screen_number) {
    uint8_t active_tilts = 0;
    uint8_t displayed_tilts = 0;

    // Clear out the display before we start printing to it
    clear();

    // Display the header row
    print_line("Color", "Gravity", 1);

    // Loop through each of the tilt colors cached by tilt_scanner, searching for active tilts
    for(uint8_t i = 0;i<TILT_COLORS;i++) {
        if(tilt_scanner.tilt(i)->is_loaded()) {
            active_tilts++;
            // This check has the added bonus of limiting the # of displayed tilts to TILTS_PER_PAGE
            if((active_tilts/TILTS_PER_PAGE)==screen_number) {
                print_tilt_to_line(tilt_scanner.tilt(i), displayed_tilts+2);
                displayed_tilts++;
            }
        }
    }

    // Toggle the actual display
    display();
}


void bridge_lcd::display_wifi_connect_screen(String ap_name, String ap_pass) {
    // This screen is displayed when the user first plugs in an unconfigured TiltBridge
    clear();
    print_line("To configure, connect to", "", 1);
    print_line("this AP via WiFi:", "", 2);
    print_line("Name:", ap_name, 3);
    print_line("Pass: ", ap_pass, 4);
    display();
}

void bridge_lcd::display_wifi_success_screen(String mdns_url, String ip_address_url) {
    // This screen is displayed at startup when the TiltBridge is configured to connect to WiFi
    clear();
    print_line("Access your TiltBridge at:", "", 1);
    print_line(mdns_url, "", 2);
    print_line(ip_address_url, "", 3);
    display();
}

void bridge_lcd::display_wifi_reset_screen() {
    // When the user presses the "boot" switch, this screen appears. If the user presses the boot button a second time
    // while this screen is displayed, WiFi settings are cleared and the TiltBridge will return to displaying the
    // configuration AP at startup
    clear();
    print_line("Press the button again to", "", 1);
    print_line("disable autoconnection to", "", 2);
    print_line("and start the WiFi ", "", 3);
    print_line("configuration AP.", "", 4);
    display();
}

void bridge_lcd::display_ota_update_screen() {
    // When the user presses the "boot" switch, this screen appears. If the user presses the boot button a second time
    // while this screen is displayed, WiFi settings are cleared and the TiltBridge will return to displaying the
    // configuration AP at startup
    clear();
    print_line("The TiltBridge firmware is", "", 1);
    print_line("being updated. Please do", "", 2);
    print_line("not power down or reset", "", 3);
    print_line("your TiltBridge.", "", 4);
    display();
}


void bridge_lcd::print_tilt_to_line(tiltHydrometer* tilt, uint8_t line) {
    char gravity[10];
    sprintf(gravity, "%.3f", double_t(tilt->gravity)/1000);
    print_line(tilt->color_name().c_str(), gravity, line);
}

/////////// LCD Wrapper Functions

void bridge_lcd::init() {
    oled_display = new TFT_eSPI(135, 240);
    oled_display->init();
    oled_display->fontHeight(2);
    oled_display->setRotation(1);
    oled_display->fillScreen(TFT_BLACK);
}

void bridge_lcd::clear() {
    oled_display->fillScreen(TFT_BLACK);
}

void bridge_lcd::display() {
}


void bridge_lcd::print_line(String left_text, String right_text, uint8_t line) {
    int16_t starting_pixel_row = 0;

    starting_pixel_row = (4 + 25) * (line-1) + 4;

    // The coordinates define the left starting point of the text
    //oled_display->setTextAlignment(TEXT_ALIGN_LEFT);
    oled_display->drawString(left_text, 0, starting_pixel_row, 4);

    //oled_display->setTextAlignment(TEXT_ALIGN_RIGHT);
    oled_display->drawString(right_text, oled_display->width()/2, starting_pixel_row, 4);
}

