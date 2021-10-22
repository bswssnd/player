#include <stdlib.h>

#include "bittu_jni_AudioPlayerJNI.h"
#include "player.h"

JNIEXPORT void JNICALL Java_bittu_jni_AudioPlayerJNI_init
  (JNIEnv * env, jobject this)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, this), "ptr", "J");

    // make assumption that NULL is zero.
    (*env)->SetLongField(env, this, f_ptr, player_init());
}

JNIEXPORT jint JNICALL Java_bittu_jni_AudioPlayerJNI_stream
  (JNIEnv * env, jobject this, jstring url)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, this), "ptr", "J");

    player_t* ptr = (*env)->GetLongField(env, this, f_ptr);
    const char* url_ = (*env)->GetStringUTFChars(env, url, NULL);

    int ret = player_stream(ptr, url_);

    (*env)->ReleaseStringUTFChars(env, url, url_);
    return -ret;
}

JNIEXPORT jint JNICALL Java_bittu_jni_AudioPlayerJNI_get20ms
  (JNIEnv * env, jobject this, jbyteArray buf)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, this), "ptr", "J");

    player_t* ptr = (*env)->GetLongField(env, this, f_ptr);
    jbyte* out = (*env)->GetByteArrayElements(env, buf, NULL);

    size_t out_size; // PLAYER_AUDIO_BUFFER_MAX_SIZE size is mandatory
    int ret = player_get_20ms_audio(ptr, out, &out_size);

    (*env)->ReleaseByteArrayElements(env, buf, out, 0);
    return ret == PLAYER_NO_ERROR ? out_size : -ret;
}

JNIEXPORT void JNICALL Java_bittu_jni_AudioPlayerJNI_uninit
  (JNIEnv * env, jobject this)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, this), "ptr", "J");

    player_uninit((*env)->GetLongField(env, this, f_ptr));
}