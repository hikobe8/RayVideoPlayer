package com.ray.util;

import android.media.MediaCodecList;

import java.util.HashMap;
import java.util.Map;

/***
 *  Author : ryu18356@gmail.com
 *  Create at 2018-10-31 17:05
 *  description : 
 */
public class VideoSupportUtil {

    private static Map<String, String> CODEC_MAP;

    static {
        CODEC_MAP = new HashMap<>();
        CODEC_MAP.put("h264", "video/avc");
    }

    public static String findVideoCodecName(String ffCodecName) {
        if (CODEC_MAP.containsKey(ffCodecName)) {
            return CODEC_MAP.get(ffCodecName);
        }
        return "";
    }

    public static boolean isSupportHardwareDecode(String ffCodecName){
        boolean support = false;
        int count = MediaCodecList.getCodecCount();
        for (int i = 0; i < count; i++) {
            String[] types = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();
            for (String type : types) {
                if (type.equals(findVideoCodecName(ffCodecName))) {
                    support = true;
                    break;
                }
            }
            if (support)
                break;
        }
        return support;
    }

}
