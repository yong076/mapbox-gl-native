package com.mapbox.mapboxgl.lib;

class MapChange {
    static int REGION_WILL_CHANGE = 0;
    static int REGION_WILL_CHANGE_ANIMATED = 1;
    static int REGION_DID_CHANGE = 2;
    static int REGION_DID_CHANGE_ANIMATED = 3;
    static int WILL_START_LOADING_MAP = 4;
    static int DID_FINISH_LOADING_MAP = 5;
    static int DID_FAIL_LOADING_MAP = 6;
    static int WILL_START_RENDERING_MAP = 7;
    static int DID_FINISH_RENDERING_MAP = 8;
    static int DID_FINISH_RENDERING_MAP_FULLY_RENDERED = 9;
}
