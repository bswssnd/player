#include "bittu_AudioPlayer.h"
#include "player.h"

JNIEXPORT void JNICALL Java_bittu_AudioPlayer_init
  (JNIEnv * env, jobject this)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, env), "ptr", "J");

    // make assumption that NULL is zero.
    (*env)->SetLongField(env, this, f_ptr, player_init());
}

JNIEXPORT jint JNICALL Java_bittu_AudioPlayer_stream
  (JNIEnv * env, jobject this, jstring url)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, env), "ptr", "J");

    player_t* ptr = (*env)->GetLongField(env, this, f_ptr);
    const char* url = (*env)->GetStringUTFChars(env, url, NULL);

    int ret = player_stream(ptr, url);

    ReleaseStringUTFChars(url);
    return -ret;
}

JNIEXPORT jint JNICALL Java_bittu_AudioPlayer_get20ms
  (JNIEnv * env, jobject this, jbyteArray buf)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, env), "ptr", "J");

    player_t* ptr = (*env)->GetLongField(env, this, f_ptr);
    jbyte* out = (*env)->GetByteArrayElements(env, buf, NULL);

    size_t out_size; // PLAYER_AUDIO_BUFFER_MAX_SIZE size is mandatory
    int ret = player_get_20ms_audio(ptr, out, out_size);

    (*env)->ReleaseByteArrayElements(env, buf, out, 0);
    return ret == PLAYER_NO_ERROR ? out_size : -ret;
}

JNIEXPORT void JNICALL Java_bittu_AudioPlayer_uninit
  (JNIEnv * env, jobject this)
{
    jfieldID f_ptr = (*env)->GetFieldID(
        env, (*env)->GetObjectClass(env, env), "ptr", "J");
    
    player_uninit((*env)->GetLongField(env, this, f_ptr));
}