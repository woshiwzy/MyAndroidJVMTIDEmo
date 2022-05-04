#include "utils.h"
#include "jvmti.h"
#include "dexer/slicer/dex_ir_builder.h"
#include "dexer/slicer/code_ir.h"
#include "dexer/slicer/reader.h"
#include "dexer/slicer/writer.h"

#define JVM_TI_CLASS "com/sand/apm/JVMTIHelper"

using namespace std;
using namespace dex;
using namespace lir;

static jvmtiEnv *localJvmtiEnv;
static int methodCount=0;
static int allocateObjCount=0;

static JavaVM *globalVm;
static jclass helperClass;

void SetAllCapabilities(jvmtiEnv *jvmti) {
    jvmtiCapabilities caps;
    jvmtiError error;
    error = jvmti->GetPotentialCapabilities(&caps);
    error = jvmti->AddCapabilities(&caps);
}


jvmtiEnv *CreateJvmtiEnv(JavaVM *vm) {
    jvmtiEnv *jvmti_env;
    jint result = vm->GetEnv((void **) &jvmti_env, JVMTI_VERSION_1_2);
    if (result != JNI_OK) {
        return nullptr;
    }

    return jvmti_env;

}

/**
 * 对象分配时回调
 * @param jvmti
 * @param jni
 * @param thread
 * @param object
 * @param klass
 * @param size
 */
void onObjectAllocCallback(jvmtiEnv *jvmti, JNIEnv *jni,jthread thread, jobject object,jclass klass, jlong size) {

    allocateObjCount++;

    jclass cls = jni->FindClass("java/lang/Class");
    jmethodID mid_getName = jni->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    jstring name = static_cast<jstring>(jni->CallObjectMethod(klass, mid_getName));
    const char* className=jni->GetStringUTFChars(name,JNI_FALSE);
    LOGI("alloc object:%s {size:%lld}, total obj count: %d", className, size,allocateObjCount);

    jni->ReleaseStringUTFChars(name,className);
}

/**
 * 方法进入
 * @param jvmti_env
 * @param jni_env
 * @param thread
 * @param method
 */
 void   onMethodEntry(jvmtiEnv *jvmti_env,JNIEnv* jni_env,jthread thread,jmethodID method) {

    jvmtiError error;
    jclass clazz;
    char* name;
    char* signature;

    // get declaring class of the method
    error = jvmti_env->GetMethodDeclaringClass(method, &clazz);
//    // get the signature of the class
    error = jvmti_env->GetClassSignature(clazz, &signature, 0);
//    // get method name
    error = jvmti_env->GetMethodName(method, &name, NULL, NULL);
//    int ret=strcmp(name,"getName");
    methodCount++;
    LOGI("第 %d 个方法 进入 %s ",methodCount,name);

    //===============================以下代码咱不使用===============================================

    //调用class 的 getName 方法获取当前执行的是哪个类的方法，但是这会引起，递归调用
//    if(strcmp(name,"getName")!=0){
////        jclass cls = jni_env->FindClass("java/lang/Class");
////        jmethodID mid_getName = jni_env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
////        jstring className = static_cast<jstring>(jni_env->CallObjectMethod(clazz, mid_getName));
////        const char *str =jni_env->GetStringUTFChars( className, 0);
////        LOGI("onMethodEntry =>:%s.%s",str,name);
//        LOGI("onMethodEntry %s 结果 :%d",name,ret);
//    } else{
//        return;
//    }

//    jvmtiFrameInfo frames[5];
//    jint count;
//    jvmtiError err;
//    err=jvmti_env->GetStackTrace(thread,0,5,frames,&count);
//    if (err == JVMTI_ERROR_NONE && count >= 1) {
//        char *methodName;
//        jvmti_env->GetMethodName(frames[0].method,&methodName, NULL, NULL);
////        err = (*jvmti)->GetMethodName(jvmti, frames[0].method,&methodName, NULL, NULL);
//        if (err == JVMTI_ERROR_NONE) {
//            printf("Executing method: %s", methodName);
//        }
//    }

////    char tmp[1024];
////    sprintf(tmp, "%s%s", signature, name);
////    LOGI("method name:%s",name);
//    LOGI("onMethodEntry=> %s ",name);

}

/**
 * 方法结束
 * @param jvmti_env
 * @param jni_env
 * @param thread
 * @param method
 * @param was_popped_by_exception
 * @param return_value
 */
void   onMethodExit(jvmtiEnv *jvmti_env,JNIEnv* jni_env,jthread thread,jmethodID method,jboolean was_popped_by_exception,jvalue return_value){
    jvmtiError error;
    char* name;
    error = jvmti_env->GetMethodName(method, &name, NULL, NULL);
    LOGI("onMethodExit %s",name);
}


/**
 * GC start
 * @param jvmti
 */
void onGCStartCallback(jvmtiEnv *jvmti) {
    LOGI("触发 GCStart");

//    JNIEnv *env;
//    if(globalVm && helperClass && (globalVm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK)){
//        jmethodID methodid = env->GetStaticMethodID(helperClass, "onGcStart","()V");
//        env->CallStaticVoidMethod(helperClass,methodid);
//    }

}


/**
 * GC 结束
 * @param jvmti
 */
void onGCFinishCallback(jvmtiEnv *jvmti) {

    bool envOk= (nullptr!=globalVm) && (nullptr!=helperClass);
    LOGI("onGCFinishCallback ready:%d",envOk);

    LOGI("触发 1.GCFinish.VM:%d",globalVm);
    LOGI("触发 2.GCFinish.HelperClass:%ld",helperClass);

    JNIEnv *env;
    if(globalVm->GetEnv((void **) &env, JNI_VERSION_1_6)==JNI_OK){
        jclass jc = env->FindClass(JVM_TI_CLASS);
//        jclass jc = env->FindClass("com/sand/apm/JVMTIHelper");
        if(nullptr!=jc){
            LOGI("找找到类");
            jmethodID methodid = env->GetStaticMethodID(jc, "onGcFinishCallBack","()V");
            env->CallStaticVoidMethod(jc,methodid);
        }else{
            LOGI("没有到类");
        }

    }

//    if(envOk && (globalVm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK)){
//        jmethodID methodid = env->GetStaticMethodID(helperClass, "onGcFinish","()V");
//        env->CallStaticVoidMethod(helperClass,methodid);
//    }

}

/**
 * 线程开始
 * @param jvmti_env
 * @param jni_env
 * @param thread
 */
void onThreadStart(jvmtiEnv *jvmti_env,JNIEnv* jni_env,jthread thread){
    LOGI("onThreadStart==>>");
}


/**
 * 线程结束
 * @param jvmti_env
 * @param jni_env
 * @param thread
 */
void onThreadEnd(jvmtiEnv *jvmti_env,JNIEnv* jni_env,jthread thread){
    LOGI("onThreadEnd==>>");

}


/**
 * 注册事件监听
 * @param jvmti
 * @param mode
 * @param event_type
 */
void SetEventNotification(jvmtiEnv *jvmti, jvmtiEventMode mode, jvmtiEvent event_type) {
    jvmtiError err = jvmti->SetEventNotificationMode(mode, event_type, nullptr);
}



/**
 * 一个用于测试的jni方法
 */
extern "C" JNIEXPORT void JNICALL doCall(JNIEnv *env,jclass clazz) {
    LOGI("触发 doCall=======");
}


/**
 * JNI绑定回调
 * @param jvmti_env
 * @param jni_env
 * @param thread
 * @param method
 * @param address
 * @param new_address_ptr
 */
void JNICALL JvmTINativeMethodBind(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method,void *address, void **new_address_ptr) {
    LOGI("JvmTINativeMethodBind");

    jclass clazz = jni_env->FindClass(JVM_TI_CLASS);
    jmethodID methodid = jni_env->GetStaticMethodID(clazz, "doCall","()V");
    if (methodid == method) {
        *new_address_ptr = reinterpret_cast<void *>(&doCall);
    }
    //绑定 package code 到BootClassLoader 里
    jfieldID packageCodePathId = jni_env->GetStaticFieldID(clazz, "packageCodePath","Ljava/lang/String;");
    jstring packageCodePath = static_cast<jstring>(jni_env->GetStaticObjectField(clazz,packageCodePathId));
    const char *pathChar = jni_env->GetStringUTFChars(packageCodePath, JNI_FALSE);
    LOGI("add to boot classloader %s===============", pathChar);
    jvmti_env->AddToBootstrapClassLoaderSearch(pathChar);
    jni_env->ReleaseStringUTFChars(packageCodePath,pathChar);

}



/**
 * Agent attch 回调
 */
extern "C" JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *vm, char *options,void *reserved) {
    //VM 在这里赋值才有效，在onLoad方法里赋值，使用的时候变成了null
    LOGI("JVM Agent_OnLoad: %d ,pid: %d",globalVm,getpid());
    LOGI("JVM Agent_OnAttach: %d ,pid: %d",vm,getpid());
    ::globalVm=vm;

    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    LOGI("Find helper class on onattch%s",JVM_TI_CLASS);
    LOGI("Classs Exist:%d", helperClass);

//    ::helperClass = env->FindClass(JVM_TI_CLASS);
    //================================================

    jvmtiEnv *jvmti_env = CreateJvmtiEnv(vm);

    if (jvmti_env == nullptr) {
        return JNI_ERR;
    }
    localJvmtiEnv = jvmti_env;
    SetAllCapabilities(jvmti_env);

    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.VMObjectAlloc = &onObjectAllocCallback;//绑定内存分配
    callbacks.NativeMethodBind = &JvmTINativeMethodBind;//

    callbacks.GarbageCollectionStart = &onGCStartCallback;//GC 开始
    callbacks.GarbageCollectionFinish = &onGCFinishCallback; //GC 结束

    callbacks.MethodEntry=&onMethodEntry;
    callbacks.MethodExit=&onMethodExit;

    callbacks.ThreadStart=&onThreadStart;
    callbacks.ThreadEnd=&onThreadEnd;


    int error = jvmti_env->SetEventCallbacks(&callbacks, sizeof(callbacks));

    //启用各种回调事件，否则可能不会触发回调方法
    SetEventNotification(jvmti_env, JVMTI_ENABLE,JVMTI_EVENT_GARBAGE_COLLECTION_START);//监听GC 开始
    SetEventNotification(jvmti_env, JVMTI_ENABLE,JVMTI_EVENT_GARBAGE_COLLECTION_FINISH);//监听GC 结束
    SetEventNotification(jvmti_env, JVMTI_ENABLE,JVMTI_EVENT_NATIVE_METHOD_BIND);//监听native method bind
    SetEventNotification(jvmti_env, JVMTI_ENABLE,JVMTI_EVENT_VM_OBJECT_ALLOC);//监听对象分配
    SetEventNotification(jvmti_env, JVMTI_ENABLE,JVMTI_EVENT_OBJECT_FREE);//监听对象释放
    SetEventNotification(jvmti_env, JVMTI_ENABLE,JVMTI_EVENT_CLASS_FILE_LOAD_HOOK);//监听类文件加载
    SetEventNotification(jvmti_env,JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY);//方法进入
    SetEventNotification(jvmti_env,JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT);//方法退出

    SetEventNotification(jvmti_env,JVMTI_ENABLE, JVMTI_EVENT_THREAD_START);//线程开始
    SetEventNotification(jvmti_env,JVMTI_ENABLE, JVMTI_EVENT_THREAD_END);//线程结束

    LOGI("==========Agent_OnAttach=======");
    return JNI_OK;

}


static JNINativeMethod methods[] = {
        {"doCall", "()V", reinterpret_cast<void *>(doCall)}
};


JNIEXPORT jint JNICALL JNI_On(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    LOGI("==============library load JNI_On====================");
    jclass clazz = env->FindClass(JVM_TI_CLASS);

    env->RegisterNatives(clazz, methods, 1);

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved){
    LOGI("JNI_OnUnload call");

}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    LOGI("Onload JVM: %d, process:%d",vm,getpid());

    LOGI("==============library load JNI_OnLoad====================");

    globalVm=vm;
    jclass helperClass = env->FindClass(JVM_TI_CLASS);
    env->RegisterNatives(helperClass, methods, 1);

    LOGE("JNI onLoad 函数中找到的class 指针:  %ld",helperClass);
    ::helperClass=helperClass;

//    ::helperClass = static_cast<jclass>(env->NewGlobalRef(helperClass));

    return JNI_VERSION_1_6;
}
