package com.ray.type;

import android.support.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Author : hikobe8@github.com
 * Time : 2018/8/19 下午3:38
 * Description :
 */
@Retention(RetentionPolicy.SOURCE)
@IntDef({
   ChannelType.LEFT,
   ChannelType.RIGHT,
   ChannelType.ALL
})
public @interface ChannelType {
    //左声道
    int LEFT = 0;
    //右声道
    int RIGHT = 1;
    //立体声
    int ALL = 2;
}
