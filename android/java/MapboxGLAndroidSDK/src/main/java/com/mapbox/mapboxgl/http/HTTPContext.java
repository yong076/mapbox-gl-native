package com.mapbox.mapboxgl.http;

import com.mapbox.mapboxgl.constants.MapboxConstants;
import com.squareup.okhttp.Callback;
import com.squareup.okhttp.OkHttpClient;
import com.squareup.okhttp.Request;
import com.squareup.okhttp.Response;

import java.io.IOException;

public class HTTPContext {

    private static HTTPContext mInstance = null;

    private OkHttpClient mClient;

    static {
        System.loadLibrary("mapbox-gl");
    }

    private HTTPContext() {
        super();
        mClient = new OkHttpClient();
    }

    public static HTTPContext getInstance() {
        if (mInstance == null) {
            mInstance = new HTTPContext();
        }

        return mInstance;
    }

    public HTTPRequest createRequest(long nativePtr, String resourceUrl, String userAgent, String etag, String modified) {
        return new HTTPRequest(nativePtr, resourceUrl, userAgent, etag, modified);
    }

    public class HTTPRequest implements Callback {

        private long mNativePtr = 0;

        private native void nativeOnFailure();
        private native void nativeOnResponse(int code, String message, String etag, String mofified, String expires, byte[] body);

        private HTTPRequest(long nativePtr, String resourceUrl, String userAgent, String etag, String modified) {
            mNativePtr = nativePtr;
            Request.Builder builder = new Request.Builder().url(resourceUrl).tag(resourceUrl.toLowerCase(MapboxConstants.MAPBOX_LOCALE)).addHeader("User-Agent", userAgent);
            if (!etag.isEmpty()) {
                builder = builder.addHeader("If-None-Match", etag);
            } else if (!modified.isEmpty()) {
                builder = builder.addHeader("If-Modified-Since", modified);
            }
            Request request = builder.build();
            HTTPContext.getInstance().mClient.newCall(request).enqueue(this);
        }

        @Override
        public void onFailure(Request request, IOException e) {
            // TODO calback into JNI
        }

        @Override
        public void onResponse(Response response) throws IOException {
            // TODO callback into JNI
            String expires = response.header("Cache-Control");
            if (expires == null) {
                expires = response.header("Expires");
            }

            byte[] body = response.body().bytes();
            response.body().close();

            nativeOnResponse(response.code(), response.message(), response.header("ETag"), response.header("Last-Modified"), expires, body);
        }
    }
}
