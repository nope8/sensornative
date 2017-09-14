#include <dlfcn.h>
#include <jni.h>
#include <string>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <android/sensor.h>
#include <android/log.h>

#define LOG_TAG     "SENSORNATIVE"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef struct _sensor_native{
    ASensorEventQueue* queue;
    ASensorManager *manager;
    ASensor const *Sensor;
    ALooper * looper;
    int32_t rate;
    int32_t max_delay;
    int32_t status;
    int count;
    ASensorList list;
}SensorNative;


SensorNative * msn;

ASensorManager* AcquireASensorManagerInstance(void) {
    typedef ASensorManager *(*PF_GETINSTANCEFORPACKAGE)(const char *name);
    void* androidHandle = dlopen("libandroid.so", RTLD_NOW);
    PF_GETINSTANCEFORPACKAGE getInstanceForPackageFunc = (PF_GETINSTANCEFORPACKAGE)
            dlsym(androidHandle, "ASensorManager_getInstanceForPackage");
    if (getInstanceForPackageFunc) {
        return getInstanceForPackageFunc("sensornative");
    }

    typedef ASensorManager *(*PF_GETINSTANCE)();
    PF_GETINSTANCE getInstanceFunc = (PF_GETINSTANCE)
            dlsym(androidHandle, "ASensorManager_getInstance");
    // by all means at this point, ASensorManager_getInstance should be available
    return getInstanceFunc();
}


int sensor_list_show(SensorNative *msn)
{
    LOGI("\tName\t\t\t StringType\t Type\t Vendor\t ReportMode\t IsWakeUp\t Resolution\t MinDelay\t MaxFifo\t FifoReserved\n ");
    for(int i = 0; i < msn->count; i++)
    {
        LOGI("\t%s\t\t\t %s\t %d\t %s\t %d\t %d\t %f\t %d\t %d\t %d\t\n",
               ASensor_getName(msn->list[i]), ASensor_getStringType(msn->list[i]), \
			ASensor_getType(msn->list[i]), ASensor_getVendor(msn->list[i]), \
			ASensor_getReportingMode(msn->list[i]) ? 1 : 0,ASensor_isWakeUpSensor(msn->list[i]), \
			ASensor_getResolution(msn->list[i]), ASensor_getMinDelay(msn->list[i]),	\
			ASensor_getFifoMaxEventCount(msn->list[i]), ASensor_getFifoReservedEventCount(msn->list[i]));
    }
    return 0;
}


int sensor_callback(int fd, int events, void *data)
{
    SensorNative * msn = (SensorNative *)data;

    //LOGI("[%s|%d]: %s call!\n", __FILE__, __LINE__, __func__);
    LOGI("fd:%d\n", fd);

    if(events == ALOOPER_EVENT_INPUT)
    {
        ASensorEvent mSensorEvent;
        while(ASensorEventQueue_getEvents(msn->queue, &mSensorEvent, 1) > 0)
        {
            if(mSensorEvent.type == ASENSOR_TYPE_ACCELEROMETER)
            {
                LOGI("acc: %f\t %f\t %f\n", mSensorEvent.acceleration.x, mSensorEvent.acceleration.y, mSensorEvent.acceleration.z);
            }
        }
    }

    return 1;
}

int sensor_stop()
{
    ASensorEventQueue_disableSensor(msn->queue, msn->Sensor);
    ASensorManager_destroyEventQueue(msn->manager, msn->queue);
    return 0;
}

//void *thread_callback(void *arg)
int sensor_start()
{
    LOGI("sensor_start start");
    int mSensorType = 1;
    int ret = 0;
    LOGI("%s|%d start", __func__, __LINE__);

    msn = (SensorNative *)calloc(1, sizeof(SensorNative));
    msn->rate = 10000;
    msn->max_delay = 0;
    msn->status = 1;

    LOGI("%s|%d start", __func__, __LINE__);
    msn->looper = ALooper_forThread();
    if(!msn->looper)
    {
        LOGI("ALooper_forThread failed!\n");
        msn->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        if(!msn->looper)
        {
            LOGI("ALooper_prepare failed!\n");
            return -1;
        }
    }
    LOGI("%s|%d start", __func__, __LINE__);
    msn->manager = AcquireASensorManagerInstance();

    msn->count = ASensorManager_getSensorList(msn->manager, &msn->list);
    sensor_list_show(msn);
    msn->Sensor = ASensorManager_getDefaultSensor(msn->manager, ASENSOR_TYPE_ACCELEROMETER);

    msn->queue = ASensorManager_createEventQueue(msn->manager, msn->looper, 0, sensor_callback, (void *)msn);
    if(!msn->queue)
    {
        LOGI("ASensorManager_createEventQueue failed!\n");
        return -1;
    }
    if(msn->Sensor != NULL && msn->looper != NULL)
    {
        //ret = ASensorEventQueue_registerSensor(msn->queue, msn->Sensor, msn->rate, msn->max_delay);
        //if(ret < 0)
        {
            //LOGI("ASensorEventQueue_registerSensor failed!\n");

            ASensorEventQueue_setEventRate(msn->queue, msn->Sensor, msn->rate);
            ret = ASensorEventQueue_enableSensor(msn->queue, msn->Sensor);
            if(ret < 0)
            {
                LOGI("ASensorEventQueue_enableSensor failed!\n");
            }
            else
                LOGI("ASensorEventQueue_enableSensor success!\n");
        }
        //else
        //    LOGI("ASensorEventQueue_registerSensor success!\n");
    }

#ifdef POLLONCE
    do{
		int ident = ALooper_pollOnce(100, NULL, NULL, NULL);
		switch(ident){
			case ALOOPER_POLL_WAKE:
				LOGI("ALOOPER_POLL_WAKE\n");
				msn->status = 1;
				break;
			case ALOOPER_POLL_CALLBACK:
				LOGI("ALOOPER_POLL_CALLBACK\n");
				msn->status = 1;
				break;
			case ALOOPER_POLL_TIMEOUT:
				LOGI("ALOOPER_POLL_TIMEOUT\n");
				msn->status = 1;
				break;
			case ALOOPER_POLL_ERROR:
				LOGI("ALOOPER_POLL_ERROR\n");
				msn->status = 0;
				break;
			default:
				break;

		}


	}while(msn->status);

#endif
    LOGI("sensor_start end");
    return 0;
}

extern "C"{
JNIEXPORT jstring JNICALL
Java_com_android_sensornative_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_android_sensornative_MainActivity_sensorStart(
        JNIEnv *env,
        jobject /* this */) {
    LOGI("Java_com_android_sensornative_MainActivity_sensorStart");
    std::string hello = "sensorStart";
    sensor_start();
    return env->NewStringUTF(hello.c_str());
}


JNIEXPORT jstring JNICALL
Java_com_android_sensornative_MainActivity_sensorStop(
        JNIEnv *env,
        jobject /* this */) {
    LOGI("Java_com_android_sensornative_MainActivity_sensorStop");
    std::string hello = "sensorStop";
    sensor_stop();
    return env->NewStringUTF(hello.c_str());
}
}