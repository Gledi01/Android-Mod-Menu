#include "Jni.hpp"
#include <list>
#include <vector>
#include <cstring>
#include <string>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <dlfcn.h>
#include "Includes/obfuscate.h"
#include "Menu/Jni.hpp"
#include "Includes/Logger.h"

void Dialog(JNIEnv *env, jobject context, const char *title, const char *message, const char *openBtn, const char *closeBtn, int sec, const char *url) {
    jclass dialogHelperClass = env->FindClass(OBFUSCATE("com/android/support/DialogHelper"));
    jmethodID showMethod = env->GetStaticMethodID(dialogHelperClass, OBFUSCATE("showDialogWithLink"),
                                                  OBFUSCATE("(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)V"));

    jstring jTitle = env->NewStringUTF(title);
    jstring jMessage = env->NewStringUTF(message);
    jstring jOpen = env->NewStringUTF(openBtn);
    jstring jClose = env->NewStringUTF(closeBtn);
    jint jSec = sec;
    jstring jUrl = env->NewStringUTF(url);

    env->CallStaticVoidMethod(dialogHelperClass, showMethod, context, jTitle, jMessage, jOpen, jClose, jSec, jUrl);

    env->DeleteLocalRef(jTitle);
    env->DeleteLocalRef(jMessage);
    env->DeleteLocalRef(jOpen);
    env->DeleteLocalRef(jClose);
    env->DeleteLocalRef(jUrl);
}

void Toast(JNIEnv *env, jobject thiz, const char *text, int length) {
    jstring jstr = env->NewStringUTF(text);
    jclass toast = env->FindClass(OBFUSCATE("android/widget/Toast"));
    jmethodID methodMakeText =env->GetStaticMethodID(toast,OBFUSCATE("makeText"),OBFUSCATE("(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;"));
    jobject toastobj = env->CallStaticObjectMethod(toast, methodMakeText,thiz, jstr, length);
    jmethodID methodShow = env->GetMethodID(toast, OBFUSCATE("show"), OBFUSCATE("()V"));
    env->CallVoidMethod(toastobj, methodShow);
}

//Big letter cause crash
void setText(JNIEnv *env, jobject obj, const char* text){
    //https://stackoverflow.com/a/33627640/3763113
    //A little JNI calls here. You really really need a great knowledge if you want to play with JNI stuff
    //Html.fromHtml("");
    jclass html = (*env).FindClass(OBFUSCATE("android/text/Html"));
    jmethodID fromHtml = (*env).GetStaticMethodID(html, OBFUSCATE("fromHtml"), OBFUSCATE("(Ljava/lang/String;)Landroid/text/Spanned;"));

    //setText("");
    jclass textView = (*env).FindClass(OBFUSCATE("android/widget/TextView"));
    jmethodID setText = (*env).GetMethodID(textView, OBFUSCATE("setText"), OBFUSCATE("(Ljava/lang/CharSequence;)V"));

    //Java string
    jstring jstr = (*env).NewStringUTF(text);
    (*env).CallVoidMethod(obj, setText,  (*env).CallStaticObjectMethod(html, fromHtml, jstr));
}

void startService(JNIEnv *env, jobject ctx){
    jclass native_context = env->GetObjectClass(ctx);
    jclass intentClass = env->FindClass(OBFUSCATE("android/content/Intent"));
    jclass actionString = env->FindClass(OBFUSCATE("com/android/support/Launcher"));
    jmethodID newIntent = env->GetMethodID(intentClass, OBFUSCATE("<init>"), OBFUSCATE("(Landroid/content/Context;Ljava/lang/Class;)V"));
    jobject intent = env->NewObject(intentClass,newIntent,ctx,actionString);
    jmethodID startActivityMethodId = env->GetMethodID(native_context, OBFUSCATE("startService"), OBFUSCATE("(Landroid/content/Intent;)Landroid/content/ComponentName;"));
    env->CallObjectMethod(ctx, startActivityMethodId, intent);
}

void *exit_thread(void *) {
    sleep(5);
    exit(0);
}

int get_api_sdk(JNIEnv* env) {
    jclass build_version_class = env->FindClass(OBFUSCATE("android/os/Build$VERSION"));
    jfieldID sdk_int_field = env->GetStaticFieldID(build_version_class, OBFUSCATE("SDK_INT"), OBFUSCATE("I"));
    return env->GetStaticIntField(build_version_class, sdk_int_field);
}

void startActivityPermisson(JNIEnv *env, jobject ctx){
    jclass native_context = env->GetObjectClass(ctx);
    jmethodID startActivity = env->GetMethodID(native_context, "startActivity", "(Landroid/content/Intent;)V");
    
    // Ambil package name
    jmethodID getPackageName = env->GetMethodID(native_context, "getPackageName", "()Ljava/lang/String;");
    jstring packageName = (jstring)env->CallObjectMethod(ctx, getPackageName);
    const char *pkg = env->GetStringUTFChars(packageName, 0);
    
    // Bikin URI: package:com.yourapp.mod
    std::string uriStr = "package:";
    uriStr += pkg;
    env->ReleaseStringUTFChars(packageName, pkg);
    
    jclass uriClass = env->FindClass("android/net/Uri");
    jmethodID parse = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
    jobject uri = env->CallStaticObjectMethod(uriClass, parse, env->NewStringUTF(uriStr.c_str()));
    
    // Intent langsung ke halaman izin overlay aplikasi ini (bukan daftar semua)
    jclass intentClass = env->FindClass("android/content/Intent");
    jmethodID newIntent = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
    jobject intent = env->NewObject(intentClass, newIntent, 
        env->NewStringUTF("android.settings.action.MANAGE_OVERLAY_PERMISSION"), 
        uri);
    
    // FLAG_ACTIVITY_NEW_TASK biar gak crash
    jmethodID addFlags = env->GetMethodID(intentClass, "addFlags", "(I)Landroid/content/Intent;");
    env->CallObjectMethod(intent, addFlags, 0x10000000); // FLAG_ACTIVITY_NEW_TASK
    
    env->CallVoidMethod(ctx, startActivity, intent);
}

//Needed jclass parameter because this is a static java method
void CheckOverlayPermission(JNIEnv *env, jclass thiz, jobject ctx){
    //If overlay permission option is greyed out, make sure to add android.permission.SYSTEM_ALERT_WINDOW in manifest

    LOGI(OBFUSCATE("Check overlay permission"));

    int sdkVer = get_api_sdk(env);
    if (sdkVer >= 23) { //Android 6.0
        jclass Settings = env->FindClass(OBFUSCATE("android/provider/Settings"));
        jmethodID canDraw =env->GetStaticMethodID(Settings, OBFUSCATE("canDrawOverlays"), OBFUSCATE("(Landroid/content/Context;)Z"));
        if (!env->CallStaticBooleanMethod(Settings, canDraw, ctx)){
            Toast(env,ctx,OBFUSCATE("Overlay permission is required in order to show mod menu."),1);
            Toast(env,ctx,OBFUSCATE("Overlay permission is required in order to show mod menu."),1);
            startActivityPermisson(env, ctx);

            pthread_t ptid;
            pthread_create(&ptid, NULL, exit_thread, NULL);
            return;
        }
    }

    LOGI(OBFUSCATE("Start service"));
    startService(env, ctx);
}
