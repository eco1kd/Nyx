#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "game/offsets.hpp"
#include "features/visuals/esp_runtime.hpp"
#include "features/aim/runtime.hpp"
#include "features/aim/assists.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_opengl3.h"
#include "ui/fonts/mozilla_text.hpp"
#include "ui/icons/lucide_vectors.hpp"
#include <cfloat>
#include <android/input.h>
#include <android/log.h>
#include <jni.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <deque>
#include <cstdint>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define LOG_TAG "LemmingRMT"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#if __SIZEOF_POINTER__ == 4
#define LR_R_TYPE(info) ELF32_R_TYPE(info)
#define LR_R_SYM(info) ELF32_R_SYM(info)
#define LR_JUMP_SLOT R_ARM_JUMP_SLOT
#define LR_GLOB_DAT R_ARM_GLOB_DAT
#elif __SIZEOF_POINTER__ == 8
#define LR_R_TYPE(info) ELF64_R_TYPE(info)
#define LR_R_SYM(info) ELF64_R_SYM(info)
#define LR_JUMP_SLOT R_AARCH64_JUMP_SLOT
#define LR_GLOB_DAT R_AARCH64_GLOB_DAT
#else
#error Unsupported pointer size.
#endif

namespace {

using SwapBuffersFn = EGLBoolean (*)(EGLDisplay, EGLSurface);
using SwapBuffersWithDamageFn = EGLBoolean (*)(EGLDisplay, EGLSurface, const EGLint*, EGLint);
using InputGetEventFn = int32_t (*)(AInputQueue*, AInputEvent**);
using NativeInjectEventFn = jboolean (*)(JNIEnv*, jobject, jobject, jint);
using GetCreatedJavaVMsFn = jint (*)(JavaVM**, jsize, jsize*);

SwapBuffersFn g_realSwapBuffers = nullptr;
SwapBuffersWithDamageFn g_realSwapBuffersWithDamageKHR = nullptr;
SwapBuffersWithDamageFn g_realSwapBuffersWithDamageEXT = nullptr;
InputGetEventFn g_realInputGetEvent = nullptr;
NativeInjectEventFn g_realNativeInjectEvent = nullptr;
JavaVM* g_javaVm = nullptr;
jclass g_motionEventClass = nullptr;
jmethodID g_motionGetActionMasked = nullptr;
jmethodID g_motionGetActionIndex = nullptr;
jmethodID g_motionGetX = nullptr;
jmethodID g_motionGetY = nullptr;
std::atomic<bool> g_started{false};
std::atomic<bool> g_menuOpen{true};
std::atomic<bool> g_imguiWantsMouse{false};
std::atomic<int> g_screenWidth{0};
std::atomic<int> g_screenHeight{0};
std::atomic<int> g_physicalWidth{0};
std::atomic<int> g_physicalHeight{0};
jmethodID g_viewGetWidth = nullptr;
jmethodID g_viewGetHeight = nullptr;
std::atomic<int> g_viewWidth{0};
std::atomic<int> g_viewHeight{0};
std::atomic<int> g_viewOffsetX{0};
std::atomic<int> g_viewOffsetY{0};
jmethodID g_viewGetLocationOnScreen = nullptr;
jmethodID g_motionGetPointerId = nullptr;
jmethodID g_motionFindPointerIndex = nullptr;

std::atomic<float> g_menuLeft{0.0f}, g_menuTop{0.0f}, g_menuRight{0.0f}, g_menuBottom{0.0f};
std::atomic<float> g_wmLeft{0.0f}, g_wmTop{0.0f}, g_wmRight{0.0f}, g_wmBottom{0.0f};
std::atomic<bool> g_touchCapturedByOverlay{false};
std::atomic<int> g_overlayPointerId{-1};

inline bool isPointInMenu(float x, float y) {
    return x >= g_menuLeft.load(std::memory_order_relaxed) &&
           x <= g_menuRight.load(std::memory_order_relaxed) &&
           y >= g_menuTop.load(std::memory_order_relaxed) &&
           y <= g_menuBottom.load(std::memory_order_relaxed);
}

inline bool isPointInWatermark(float x, float y) {
    return x >= g_wmLeft.load(std::memory_order_relaxed) &&
           x <= g_wmRight.load(std::memory_order_relaxed) &&
           y >= g_wmTop.load(std::memory_order_relaxed) &&
           y <= g_wmBottom.load(std::memory_order_relaxed);
}

bool clearJniException(JNIEnv* env, const char* stage) {
    if (!env || !env->ExceptionCheck()) return false;
    env->ExceptionClear();
    LOGE("JNI exception during %s", stage ? stage : "unknown stage");
    return true;
}

JavaVM* getOrInitJavaVm() {
    if (g_javaVm) return g_javaVm;
    auto getCreatedVms = reinterpret_cast<GetCreatedJavaVMsFn>(
        dlsym(RTLD_DEFAULT, "JNI_GetCreatedJavaVMs"));
    if (!getCreatedVms) {
        const char* candidates[] = {"libart.so", "libnativehelper.so"};
        for (const char* library : candidates) {
            void* handle = dlopen(library, RTLD_NOW | RTLD_LOCAL);
            if (!handle) continue;
            getCreatedVms = reinterpret_cast<GetCreatedJavaVMsFn>(
                dlsym(handle, "JNI_GetCreatedJavaVMs"));
            if (getCreatedVms) break;
        }
    }
    if (!getCreatedVms) return nullptr;
    jsize count = 0;
    JavaVM* vm = nullptr;
    if (getCreatedVms(&vm, 1, &count) != JNI_OK || count < 1 || !vm) return nullptr;
    g_javaVm = vm;
    return vm;
}

JNIEnv* getJniEnv() {
    JavaVM* vm = getOrInitJavaVm();
    if (!vm) return nullptr;
    JNIEnv* env = nullptr;
    const jint res = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    } else if (res != JNI_OK) {
        return nullptr;
    }
    return env;
}

static jobject g_hiddenEditText = nullptr;
static jobject g_inputMethodManager = nullptr;
static jmethodID g_editTextSetText = nullptr;
static jmethodID g_editTextGetText = nullptr;
static jmethodID g_editTextSetSelection = nullptr;
static jmethodID g_editTextSetInputType = nullptr;
static jmethodID g_viewRequestFocus = nullptr;
static jmethodID g_viewClearFocus = nullptr;
static jmethodID g_viewGetWindowToken = nullptr;
static jmethodID g_immShowSoftInput = nullptr;
static jmethodID g_immHideSoftInput = nullptr;
static jmethodID g_immIsActive = nullptr;
static jmethodID g_charSequenceToString = nullptr;
static std::atomic<int> g_activeField{0};
static std::atomic<bool> g_imeEnterPressed{false};
static int g_imeGraceFrames = 0;

static jobject g_appActivity = nullptr;

jobject getActivity(JNIEnv* env) {
    if (g_appActivity) return g_appActivity;
    if (!env) return nullptr;

    jclass activityThread = env->FindClass("android/app/ActivityThread");
    if (activityThread && !clearJniException(env, "finding ActivityThread")) {
        jmethodID currentAppMethod = env->GetStaticMethodID(
            activityThread, "currentApplication", "()Landroid/app/Application;");
        if (currentAppMethod && !clearJniException(env, "finding currentApplication")) {
            jobject application = env->CallStaticObjectMethod(activityThread, currentAppMethod);
            if (application && !clearJniException(env, "getting currentApplication")) {
                jclass applicationClass = env->GetObjectClass(application);
                jmethodID getClassLoader = applicationClass ? env->GetMethodID(
                    applicationClass, "getClassLoader", "()Ljava/lang/ClassLoader;") : nullptr;
                if (getClassLoader && !clearJniException(env, "finding getClassLoader")) {
                    jobject classLoader = env->CallObjectMethod(application, getClassLoader);
                    if (classLoader && !clearJniException(env, "getting classLoader")) {
                        jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
                        jmethodID loadClass = classLoaderClass ? env->GetMethodID(
                            classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;") : nullptr;
                        jstring unityName = env->NewStringUTF("com.unity3d.player.UnityPlayer");
                        jobject unityClassObject = (loadClass && unityName) ? env->CallObjectMethod(classLoader, loadClass, unityName) : nullptr;
                        if (unityName) env->DeleteLocalRef(unityName);
                        if (unityClassObject && !clearJniException(env, "loading UnityPlayer class")) {
                            jclass unityClass = reinterpret_cast<jclass>(unityClassObject);
                            jfieldID currentActivityField = env->GetStaticFieldID(
                                unityClass, "currentActivity", "Landroid/app/Activity;");
                            if (currentActivityField && !clearJniException(env, "finding UnityPlayer.currentActivity")) {
                                jobject act = env->GetStaticObjectField(unityClass, currentActivityField);
                                if (act && !clearJniException(env, "getting UnityPlayer.currentActivity")) {
                                    g_appActivity = env->NewGlobalRef(act);
                                    env->DeleteLocalRef(act);
                                }
                            }
                            env->DeleteLocalRef(unityClassObject);
                        }
                        if (classLoaderClass) env->DeleteLocalRef(classLoaderClass);
                        env->DeleteLocalRef(classLoader);
                    }
                }
                if (applicationClass) env->DeleteLocalRef(applicationClass);
                env->DeleteLocalRef(application);
            }
        }
        env->DeleteLocalRef(activityThread);
    }
    return g_appActivity;
}

void setupImeBridge(JNIEnv* env, jobject viewOrActivity) {
    if (!env || !viewOrActivity || g_hiddenEditText) return;

    jclass activityClass = env->FindClass("android/app/Activity");
    jclass viewClass = env->FindClass("android/view/View");
    jclass viewGroupClass = env->FindClass("android/view/ViewGroup");
    if (!viewClass || !viewGroupClass) {
        clearJniException(env, "finding View/ViewGroup classes");
        return;
    }

    jobject context = nullptr;
    jobject parentViewGroup = nullptr;

    const bool isActivity = activityClass && env->IsInstanceOf(viewOrActivity, activityClass);
    clearJniException(env, "checking isActivity");

    if (isActivity) {
        if (!g_appActivity) {
            g_appActivity = env->NewGlobalRef(viewOrActivity);
        }
        context = viewOrActivity;
        jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
        if (getWindow && !clearJniException(env, "finding getWindow")) {
            jobject window = env->CallObjectMethod(viewOrActivity, getWindow);
            if (window && !clearJniException(env, "calling getWindow")) {
                jclass windowClass = env->GetObjectClass(window);
                jmethodID getDecorView = windowClass ? env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;") : nullptr;
                if (getDecorView && !clearJniException(env, "finding getDecorView")) {
                    jobject decor = env->CallObjectMethod(window, getDecorView);
                    clearJniException(env, "calling getDecorView");
                    parentViewGroup = decor;
                }
                if (windowClass) env->DeleteLocalRef(windowClass);
                env->DeleteLocalRef(window);
            }
        }
    } else {
        jmethodID getContext = env->GetMethodID(viewClass, "getContext", "()Landroid/content/Context;");
        if (getContext && !clearJniException(env, "finding getContext")) {
            context = env->CallObjectMethod(viewOrActivity, getContext);
            clearJniException(env, "calling getContext");
        }
        if (env->IsInstanceOf(viewOrActivity, viewGroupClass)) {
            parentViewGroup = viewOrActivity;
        }
    }

    if (!context || !parentViewGroup) {
        if (context && context != viewOrActivity) env->DeleteLocalRef(context);
        if (parentViewGroup && parentViewGroup != viewOrActivity) env->DeleteLocalRef(parentViewGroup);
        if (activityClass) env->DeleteLocalRef(activityClass);
        if (viewClass) env->DeleteLocalRef(viewClass);
        if (viewGroupClass) env->DeleteLocalRef(viewGroupClass);
        return;
    }

    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getSystemService = contextClass ? env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;") : nullptr;
    jstring ims = env->NewStringUTF("input_method");
    if (getSystemService && ims && !clearJniException(env, "finding getSystemService")) {
        jobject imm = env->CallObjectMethod(context, getSystemService, ims);
        if (imm && !clearJniException(env, "calling getSystemService for input_method")) {
            g_inputMethodManager = env->NewGlobalRef(imm);
            jclass immClass = env->GetObjectClass(imm);
            if (immClass) {
                g_immShowSoftInput = env->GetMethodID(immClass, "showSoftInput", "(Landroid/view/View;I)Z");
                g_immHideSoftInput = env->GetMethodID(immClass, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
                g_immIsActive = env->GetMethodID(immClass, "isActive", "(Landroid/view/View;)Z");
                clearJniException(env, "resolving IMM methods");
                env->DeleteLocalRef(immClass);
            }
            env->DeleteLocalRef(imm);
        }
    }
    if (ims) env->DeleteLocalRef(ims);
    if (contextClass) env->DeleteLocalRef(contextClass);

    jclass editTextClass = env->FindClass("android/widget/EditText");
    if (editTextClass && !clearJniException(env, "finding EditText class")) {
        jmethodID etInit = env->GetMethodID(editTextClass, "<init>", "(Landroid/content/Context;)V");
        if (etInit && !clearJniException(env, "finding EditText constructor")) {
            jobject et = env->NewObject(editTextClass, etInit, context);
            if (et && !clearJniException(env, "instantiating EditText")) {
                g_hiddenEditText = env->NewGlobalRef(et);

                g_editTextSetText = env->GetMethodID(editTextClass, "setText", "(Ljava/lang/CharSequence;)V");
                g_editTextGetText = env->GetMethodID(editTextClass, "getText", "()Landroid/text/Editable;");
                g_editTextSetSelection = env->GetMethodID(editTextClass, "setSelection", "(I)V");
                g_editTextSetInputType = env->GetMethodID(editTextClass, "setInputType", "(I)V");
                g_viewRequestFocus = env->GetMethodID(viewClass, "requestFocus", "()Z");
                g_viewClearFocus = env->GetMethodID(viewClass, "clearFocus", "()V");
                g_viewGetWindowToken = env->GetMethodID(viewClass, "getWindowToken", "()Landroid/os/IBinder;");
                clearJniException(env, "resolving EditText methods");

                jmethodID setAlpha = env->GetMethodID(viewClass, "setAlpha", "(F)V");
                if (setAlpha) env->CallVoidMethod(et, setAlpha, 0.0f);
                jmethodID setFocusable = env->GetMethodID(viewClass, "setFocusable", "(Z)V");
                if (setFocusable) env->CallVoidMethod(et, setFocusable, JNI_TRUE);
                jmethodID setFocusableInTouch = env->GetMethodID(viewClass, "setFocusableInTouchMode", "(Z)V");
                if (setFocusableInTouch) env->CallVoidMethod(et, setFocusableInTouch, JNI_TRUE);
                jmethodID setCursorVisible = env->GetMethodID(editTextClass, "setCursorVisible", "(Z)V");
                if (setCursorVisible) env->CallVoidMethod(et, setCursorVisible, JNI_FALSE);
                clearJniException(env, "configuring EditText properties");

                jclass lpClass = env->FindClass("android/view/ViewGroup$LayoutParams");
                jmethodID lpInit = lpClass ? env->GetMethodID(lpClass, "<init>", "(II)V") : nullptr;
                jmethodID addViewLp = env->GetMethodID(viewGroupClass, "addView", "(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V");
                if (lpClass && lpInit && addViewLp) {
                    jobject lp = env->NewObject(lpClass, lpInit, 1, 1);
                    if (lp) {
                        env->CallVoidMethod(parentViewGroup, addViewLp, et, lp);
                        clearJniException(env, "adding EditText with LayoutParams");
                        env->DeleteLocalRef(lp);
                    }
                    env->DeleteLocalRef(lpClass);
                } else {
                    jmethodID addView = env->GetMethodID(viewGroupClass, "addView", "(Landroid/view/View;)V");
                    if (addView) {
                        env->CallVoidMethod(parentViewGroup, addView, et);
                        clearJniException(env, "adding EditText without LayoutParams");
                    }
                }

                LOGI("IME bridge ready: hidden EditText created and attached");
                env->DeleteLocalRef(et);
            }
        }
        env->DeleteLocalRef(editTextClass);
    }

    jclass charSequenceClass = env->FindClass("java/lang/CharSequence");
    if (charSequenceClass && !clearJniException(env, "finding CharSequence")) {
        g_charSequenceToString = env->GetMethodID(charSequenceClass, "toString", "()Ljava/lang/String;");
        clearJniException(env, "finding toString");
        env->DeleteLocalRef(charSequenceClass);
    }

    if (context && context != viewOrActivity) env->DeleteLocalRef(context);
    if (parentViewGroup && parentViewGroup != viewOrActivity) env->DeleteLocalRef(parentViewGroup);
    if (activityClass) env->DeleteLocalRef(activityClass);
    if (viewClass) env->DeleteLocalRef(viewClass);
    if (viewGroupClass) env->DeleteLocalRef(viewGroupClass);
}

void imeShow(JNIEnv* env, int field, const char* initialText) {
    g_activeField.store(field, std::memory_order_relaxed);
    g_imeGraceFrames = 12;
    if (!env || !g_hiddenEditText) return;

    if (g_editTextSetInputType) {
        const jint inputType = (field == 2) ? 129 : 1;
        env->CallVoidMethod(g_hiddenEditText, g_editTextSetInputType, inputType);
        clearJniException(env, "setting EditText inputType");
    }

    if (g_editTextSetText && initialText) {
        jstring jtext = env->NewStringUTF(initialText);
        if (jtext) {
            env->CallVoidMethod(g_hiddenEditText, g_editTextSetText, jtext);
            clearJniException(env, "setting EditText text");
            if (g_editTextSetSelection) {
                const jint len = static_cast<jint>(std::strlen(initialText));
                env->CallVoidMethod(g_hiddenEditText, g_editTextSetSelection, len);
                clearJniException(env, "setting EditText selection");
            }
            env->DeleteLocalRef(jtext);
        }
    }

    if (g_viewRequestFocus) {
        env->CallBooleanMethod(g_hiddenEditText, g_viewRequestFocus);
        clearJniException(env, "requesting EditText focus");
    }

    if (g_inputMethodManager && g_immShowSoftInput) {
        jboolean shown = env->CallBooleanMethod(g_inputMethodManager, g_immShowSoftInput, g_hiddenEditText, 0);
        if (!shown) {
            env->CallBooleanMethod(g_inputMethodManager, g_immShowSoftInput, g_hiddenEditText, 2);
        }
        clearJniException(env, "showing soft input");
    }
}

void imeHide(JNIEnv* env) {
    g_activeField.store(0, std::memory_order_relaxed);
    g_imeGraceFrames = 0;
    if (!env || !g_hiddenEditText) return;

    if (g_viewClearFocus) {
        env->CallVoidMethod(g_hiddenEditText, g_viewClearFocus);
        clearJniException(env, "clearing EditText focus");
    }

    if (g_inputMethodManager && g_immHideSoftInput && g_viewGetWindowToken) {
        jobject token = env->CallObjectMethod(g_hiddenEditText, g_viewGetWindowToken);
        if (token) {
            if (!clearJniException(env, "getting window token")) {
                env->CallBooleanMethod(g_inputMethodManager, g_immHideSoftInput, token, 0);
                clearJniException(env, "hiding soft input");
            }
            env->DeleteLocalRef(token);
        }
    }
}

void imeSyncText(JNIEnv* env, char* targetBuffer, size_t maxLen) {
    if (!env || !g_hiddenEditText || !g_editTextGetText || !g_charSequenceToString || !targetBuffer || maxLen == 0) return;
    jobject editable = env->CallObjectMethod(g_hiddenEditText, g_editTextGetText);
    if (clearJniException(env, "getting text from EditText")) {
        if (editable) env->DeleteLocalRef(editable);
        return;
    }
    if (editable) {
        jstring str = reinterpret_cast<jstring>(env->CallObjectMethod(editable, g_charSequenceToString));
        if (clearJniException(env, "calling toString on text")) {
            if (str) env->DeleteLocalRef(str);
            env->DeleteLocalRef(editable);
            return;
        }
        if (str) {
            const char* utf = env->GetStringUTFChars(str, nullptr);
            if (utf) {
                std::snprintf(targetBuffer, maxLen, "%s", utf);
                env->ReleaseStringUTFChars(str, utf);
                size_t len = std::strlen(targetBuffer);
                while (len > 0 && (targetBuffer[len - 1] == '\n' || targetBuffer[len - 1] == '\r')) {
                    targetBuffer[--len] = '\0';
                    g_imeEnterPressed.store(true, std::memory_order_relaxed);
                }
            }
            env->DeleteLocalRef(str);
        }
        env->DeleteLocalRef(editable);
    }
}

bool imeIsActive(JNIEnv* env) {
    if (!env || !g_inputMethodManager || !g_immIsActive || !g_hiddenEditText) return false;
    jboolean act = env->CallBooleanMethod(g_inputMethodManager, g_immIsActive, g_hiddenEditText);
    clearJniException(env, "checking IMM isActive");
    return act == JNI_TRUE;
}


std::atomic<int> g_swapSlots{0};
std::atomic<int> g_swapDamageSlots{0};
std::atomic<int> g_inputSlots{0};
std::atomic<bool> g_javaInputHookReady{false};
std::atomic<uint64_t> g_javaInputEvents{0};
std::atomic<uint64_t> g_presentFrames{0};
std::atomic<uint64_t> g_renderedFrames{0};
std::atomic<uint64_t> g_renderFailures{0};
std::atomic<int> g_glClientVersion{0};
std::atomic<uintptr_t> g_lastEglContext{0};
std::atomic<uintptr_t> g_lastEglSurface{0};
std::atomic<uintptr_t> g_unityBase{0};
std::atomic<bool> g_offsetProfileReady{false};
volatile sig_atomic_t g_toggleMenuSignal = 0;
volatile sig_atomic_t g_dumpDiagnosticsSignal = 0;

EGLContext g_imguiContext = EGL_NO_CONTEXT;
EGLDisplay g_imguiDisplay = EGL_NO_DISPLAY;
EGLSurface g_imguiSurface = EGL_NO_SURFACE;
bool g_imguiBackendReady = false;
std::chrono::steady_clock::time_point g_lastImGuiFrame{};
std::atomic<bool> g_espBoxesEnabled{true};
std::atomic<bool> g_espResolverReady{false};
ImFont* g_uiTitleFont=nullptr;
ImFont* g_uiSmallFont=nullptr;

struct PointerEvent { float x; float y; int kind; };
std::mutex g_pointerMutex;
std::deque<PointerEvent> g_pointerEvents;

void queuePointerEvent(float x, float y, int kind) {
    std::lock_guard<std::mutex> lock(g_pointerMutex);
    if (g_pointerEvents.size() >= 64) g_pointerEvents.pop_front();
    g_pointerEvents.push_back({x, y, kind});
}

using EspScreenBox = lemming::esp::ScreenBox;
std::mutex g_espMutex;
std::vector<EspScreenBox> g_espScreenBoxes;

#if defined(__aarch64__)
constexpr const char* kArchitectureLabel = "ARM64 / GLES3";
#else
constexpr const char* kArchitectureLabel = "ARMV7 / GLES3";
#endif

void configureImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 12.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.WindowRounding = 15.0f;
    style.ChildRounding = 11.0f;
    style.FrameRounding = 9.0f;
    style.PopupRounding = 11.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 9.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(0.93f, 0.95f, 0.98f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.46f, 0.51f, 0.60f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.043f, 0.059f, 0.98f);
    c[ImGuiCol_Border] = ImVec4(0.16f, 0.19f, 0.25f, 0.85f);
    c[ImGuiCol_FrameBg] = ImVec4(0.075f, 0.090f, 0.122f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = c[ImGuiCol_FrameBg];
    c[ImGuiCol_FrameBgActive] = c[ImGuiCol_FrameBg];
    c[ImGuiCol_Button] = ImVec4(0.075f, 0.090f, 0.122f, 1.00f);
    c[ImGuiCol_ButtonHovered] = c[ImGuiCol_Button];
    c[ImGuiCol_ButtonActive] = c[ImGuiCol_Button];
    c[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = c[ImGuiCol_SliderGrab];
    c[ImGuiCol_Header] = ImVec4(0.12f, 0.16f, 0.22f, 1.00f);
    c[ImGuiCol_HeaderHovered] = c[ImGuiCol_Header];
    c[ImGuiCol_HeaderActive] = c[ImGuiCol_Header];
    c[ImGuiCol_Separator] = ImVec4(0.14f, 0.17f, 0.23f, 1.00f);
}

bool ensureImGui(EGLDisplay display, EGLSurface surface) {
    const EGLContext current = eglGetCurrentContext();
    if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE || current == EGL_NO_CONTEXT) {
        return false;
    }
    g_lastEglContext.store(reinterpret_cast<uintptr_t>(current), std::memory_order_relaxed);
    g_lastEglSurface.store(reinterpret_cast<uintptr_t>(surface), std::memory_order_relaxed);

    if (g_imguiBackendReady && current == g_imguiContext && display == g_imguiDisplay &&
        surface == g_imguiSurface && ImGui::GetCurrentContext()) {
        return true;
    }

    EGLint clientVersion = 0;
    if (eglQueryContext(display, current, EGL_CONTEXT_CLIENT_VERSION, &clientVersion) != EGL_TRUE) {
        clientVersion = 0;
    }
    g_glClientVersion.store(clientVersion, std::memory_order_relaxed);
    if (clientVersion < 3) {
        static EGLContext lastRejected = EGL_NO_CONTEXT;
        if (lastRejected != current) {
            lastRejected = current;
            LOGE("renderer rejected EGL context=%p: GLES client version=%d; this build requires GLES3",
                 current, clientVersion);
        }
        return false;
    }

    if (ImGui::GetCurrentContext()) {
        if (g_imguiBackendReady && current == g_imguiContext) ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
    g_imguiBackendReady = false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.BackendPlatformName = "Nyx_EGL";
    io.ConfigWindowsMoveFromTitleBarOnly = false;

    const char* sysFontCandidates[] = {
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/NotoSans-Regular.ttf"
    };
    const char* sysFontPath = nullptr;
    for (const char* candidate : sysFontCandidates) {
        if (access(candidate, R_OK) == 0) {
            sysFontPath = candidate;
            break;
        }
    }

    ImFont* font = nullptr;
    if (sysFontPath) {
        ImFontConfig cfgFile{};
        cfgFile.OversampleH = 2;
        cfgFile.OversampleV = 2;
        cfgFile.PixelSnapH = false;
        font = io.Fonts->AddFontFromFileTTF(
            sysFontPath, 21.0f, &cfgFile, io.Fonts->GetGlyphRangesCyrillic());
        if (font) {
            g_uiTitleFont = io.Fonts->AddFontFromFileTTF(
                sysFontPath, 27.0f, &cfgFile, io.Fonts->GetGlyphRangesCyrillic());
            g_uiSmallFont = io.Fonts->AddFontFromFileTTF(
                sysFontPath, 17.0f, &cfgFile, io.Fonts->GetGlyphRangesCyrillic());
            LOGI("Loaded system font: %s", sysFontPath);
        }
    }

    if (!font) {
        ImFontConfig cfgMem{};
        cfgMem.FontDataOwnedByAtlas = false;
        cfgMem.OversampleH = 3;
        cfgMem.OversampleV = 2;
        cfgMem.PixelSnapH = true;
        font = io.Fonts->AddFontFromMemoryTTF(
            const_cast<std::uint8_t*>(lemming::fonts::kMozillaText),
            static_cast<int>(lemming::fonts::kMozillaTextSize), 21.0f, &cfgMem,
            io.Fonts->GetGlyphRangesCyrillic());
        if (!font) {
            LOGW("Mozilla Text font failed; using default ImGui font");
            io.Fonts->AddFontDefault();
        }
        g_uiTitleFont = io.Fonts->AddFontFromMemoryTTF(
            const_cast<std::uint8_t*>(lemming::fonts::kMozillaText),
            static_cast<int>(lemming::fonts::kMozillaTextSize), 27.0f, &cfgMem,
            io.Fonts->GetGlyphRangesCyrillic());
        g_uiSmallFont = io.Fonts->AddFontFromMemoryTTF(
            const_cast<std::uint8_t*>(lemming::fonts::kMozillaText),
            static_cast<int>(lemming::fonts::kMozillaTextSize), 17.0f, &cfgMem,
            io.Fonts->GetGlyphRangesCyrillic());
    }
    if (!g_uiTitleFont) g_uiTitleFont = font ? font : ImGui::GetFont();
    if (!g_uiSmallFont) g_uiSmallFont = font ? font : ImGui::GetFont();
    configureImGuiStyle();

    if (!ImGui_ImplOpenGL3_Init("#version 300 es") || !ImGui_ImplOpenGL3_CreateDeviceObjects()) {
        LOGE("ImGui GLES3 backend/device initialization failed: context=%p", current);
        if (ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData) {
            ImGui_ImplOpenGL3_Shutdown();
        }
        ImGui::DestroyContext();
        return false;
    }

    g_imguiContext = current;
    g_imguiDisplay = display;
    g_imguiSurface = surface;
    g_imguiBackendReady = true;
    g_lastImGuiFrame = std::chrono::steady_clock::now();

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glsl = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    LOGI("renderer ready: context=%p GLES=%d GL=%s GLSL=%s GPU=%s",
         current, clientVersion, version ? version : "unknown", glsl ? glsl : "unknown",
         renderer ? renderer : "unknown");
    return true;
}

#include "ui/premium.hpp"

bool drawOverlay(EGLDisplay display, EGLSurface surface, int width, int height) {
    if (width < 160 || height < 120 || !ensureImGui(display, surface)) {
        g_renderFailures.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    int viewWidth = g_viewWidth.load(std::memory_order_relaxed);
    int viewHeight = g_viewHeight.load(std::memory_order_relaxed);
    if (viewWidth <= 0 || viewHeight <= 0) {
        viewWidth = width;
        viewHeight = height;
    } else if ((width > height && viewWidth < viewHeight) || (width < height && viewWidth > viewHeight)) {
        std::swap(viewWidth, viewHeight);
    }

    if (g_physicalWidth.load(std::memory_order_relaxed) <= 0 && width > 0 && height > 0) {
        int pw = width, ph = height;
        if (pw < ph) std::swap(pw, ph);
        g_physicalWidth.store(pw, std::memory_order_relaxed);
        g_physicalHeight.store(ph, std::memory_order_relaxed);
    }

    const bool espEnabled = g_espBoxesEnabled.load(std::memory_order_relaxed);
    if (espEnabled || lemming::aim::g_active.load(std::memory_order_acquire)) {
        // Start empty: no previous-frame pixel coordinates may leak into this frame.
        std::vector<EspScreenBox> nextEspBoxes;
        const bool espUpdated = lemming::esp::update(
            g_unityBase.load(std::memory_order_acquire), viewWidth, viewHeight, nextEspBoxes);
        if (!espEnabled) nextEspBoxes.clear();
        g_espResolverReady.store(espUpdated && lemming::esp::ready(), std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(g_espMutex);
            g_espScreenBoxes.swap(nextEspBoxes);
        }
    } else {
        g_espResolverReady.store(false, std::memory_order_release);
        std::lock_guard<std::mutex> lock(g_espMutex);
        g_espScreenBoxes.clear();
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(viewWidth), static_cast<float>(viewHeight));
    io.DisplayFramebufferScale = ImVec2(
        static_cast<float>(width) / static_cast<float>(viewWidth),
        static_cast<float>(height) / static_cast<float>(viewHeight));
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - g_lastImGuiFrame).count();
    io.DeltaTime = (dt > 0.0f && dt < 0.25f) ? dt : (1.0f / 60.0f);
    g_lastImGuiFrame = now;

    std::vector<PointerEvent> events;
    {
        std::lock_guard<std::mutex> lock(g_pointerMutex);
        events.assign(g_pointerEvents.begin(), g_pointerEvents.end());
        g_pointerEvents.clear();
    }

    ImGui_ImplOpenGL3_NewFrame();
    for (const auto& event : events) {
        io.AddMousePosEvent(event.x, event.y);
        if (event.kind == 1) io.AddMouseButtonEvent(0, true);
        if (event.kind == 2) io.AddMouseButtonEvent(0, false);
    }
    const int activeField = g_activeField.load(std::memory_order_relaxed);
    if (activeField != 0) {
        JNIEnv* env = getJniEnv();
        if (env) {
            if (g_imeGraceFrames > 0) {
                --g_imeGraceFrames;
                if (activeField == 1) {
                    imeSyncText(env, login_input, sizeof(login_input));
                } else if (activeField == 2) {
                    imeSyncText(env, password_input, sizeof(password_input));
                }
            } else if (!imeIsActive(env)) {
                imeHide(env);
            } else {
                if (activeField == 1) {
                    imeSyncText(env, login_input, sizeof(login_input));
                } else if (activeField == 2) {
                    imeSyncText(env, password_input, sizeof(password_input));
                }
            }
        }
    }

    ImGui::NewFrame();
    renderMinimalMenu();
    g_imguiWantsMouse.store(io.WantCaptureMouse, std::memory_order_release);

    GLboolean lastDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &lastDepthMask);
    GLboolean lastColorMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    glGetBooleanv(GL_COLOR_WRITEMASK, lastColorMask);
    GLint lastFbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastFbo);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glDepthMask(lastDepthMask);
    glColorMask(lastColorMask[0], lastColorMask[1], lastColorMask[2], lastColorMask[3]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastFbo);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        static auto lastGlErrorLog = std::chrono::steady_clock::time_point{};
        if (now - lastGlErrorLog >= std::chrono::seconds(2)) {
            lastGlErrorLog = now;
            LOGW("renderer GL error after frame: 0x%x", static_cast<unsigned>(error));
        }
    }
    g_renderedFrames.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void handleTouch(float, float) {
    // One toggle owner: ImGui watermark button on release; never toggle here.
}

[[maybe_unused]] jboolean hookedNativeInjectEvent(JNIEnv* env, jobject instance, jobject inputEvent, jint displayId) {
    if (env && instance && !g_hiddenEditText) {
        setupImeBridge(env, instance);
    }
    if (env && instance && g_viewGetWidth && g_viewGetHeight &&
        (g_viewWidth.load(std::memory_order_relaxed) <= 0 ||
         g_viewHeight.load(std::memory_order_relaxed) <= 0)) {
        const jint vw = env->CallIntMethod(instance, g_viewGetWidth);
        const jint vh = env->CallIntMethod(instance, g_viewGetHeight);
        if (!clearJniException(env, "reading View dimensions") && vw > 0 && vh > 0) {
            g_viewWidth.store(vw, std::memory_order_relaxed);
            g_viewHeight.store(vh, std::memory_order_relaxed);
            LOGI("View dimensions detected from UnityPlayer: %dx%d", vw, vh);
        }
    }

    bool shouldBlockEvent = false;

    if (env && inputEvent && g_motionEventClass &&
        env->IsInstanceOf(inputEvent, g_motionEventClass) == JNI_TRUE) {
        const jint action = env->CallIntMethod(inputEvent, g_motionGetActionMasked);
        const jint index = env->CallIntMethod(inputEvent, g_motionGetActionIndex);
        if (!clearJniException(env, "reading MotionEvent action") &&
            (action == 0 || action == 1 || action == 2 || action == 3 || action == 5 || action == 6)) {

            if (action == 0 && instance && g_viewGetWidth && g_viewGetHeight) {
                const jint vw = env->CallIntMethod(instance, g_viewGetWidth);
                const jint vh = env->CallIntMethod(instance, g_viewGetHeight);
                if (!clearJniException(env, "refreshing View dimensions") && vw > 0 && vh > 0) {
                    g_viewWidth.store(vw, std::memory_order_relaxed);
                    g_viewHeight.store(vh, std::memory_order_relaxed);
                }
                if (g_viewGetLocationOnScreen) {
                    jintArray locArray = env->NewIntArray(2);
                    if (locArray) {
                        env->CallVoidMethod(instance, g_viewGetLocationOnScreen, locArray);
                        if (!clearJniException(env, "getting View location on screen")) {
                            jint loc[2]{0, 0};
                            env->GetIntArrayRegion(locArray, 0, 2, loc);
                            g_viewOffsetX.store(loc[0], std::memory_order_relaxed);
                            g_viewOffsetY.store(loc[1], std::memory_order_relaxed);
                        }
                        env->DeleteLocalRef(locArray);
                    }
                }
            }

            jint pointerIndex = index;
            if (action == 2) {
                const int activePid = g_overlayPointerId.load(std::memory_order_relaxed);
                if (activePid >= 0 && g_motionFindPointerIndex) {
                    const jint found = env->CallIntMethod(inputEvent, g_motionFindPointerIndex, activePid);
                    if (!clearJniException(env, "finding pointer index") && found >= 0) {
                        pointerIndex = found;
                    } else {
                        pointerIndex = 0;
                    }
                } else {
                    pointerIndex = 0;
                }
            }

            const jfloat x = env->CallFloatMethod(inputEvent, g_motionGetX, pointerIndex);
            const jfloat y = env->CallFloatMethod(inputEvent, g_motionGetY, pointerIndex);
            if (!clearJniException(env, "reading MotionEvent coordinates")) {
                const uint64_t eventNumber = g_javaInputEvents.fetch_add(1, std::memory_order_relaxed) + 1;
                const int kind = (action == 0 || action == 5) ? 1 : ((action == 1 || action == 3 || action == 6) ? 2 : 0);

                int pw = g_physicalWidth.load(std::memory_order_relaxed);
                int ph = g_physicalHeight.load(std::memory_order_relaxed);
                if (pw <= 0 || ph <= 0) {
                    pw = g_screenWidth.load(std::memory_order_relaxed);
                    ph = g_screenHeight.load(std::memory_order_relaxed);
                    if (pw < ph) std::swap(pw, ph);
                }
                const int vw = g_viewWidth.load(std::memory_order_relaxed);
                const int vh = g_viewHeight.load(std::memory_order_relaxed);
                const int offX = g_viewOffsetX.load(std::memory_order_relaxed);
                const int offY = g_viewOffsetY.load(std::memory_order_relaxed);
                const float localX = x - static_cast<float>(offX);
                const float localY = y - static_cast<float>(offY);
                const float sx = (pw > 0 && vw > 0) ? (static_cast<float>(vw) / static_cast<float>(pw)) : 1.0f;
                const float sy = (ph > 0 && vh > 0) ? (static_cast<float>(vh) / static_cast<float>(ph)) : 1.0f;
                const float scaledX = (vw > 0) ? std::clamp(localX * sx, 0.0f, static_cast<float>(vw)) : x;
                const float scaledY = (vh > 0) ? std::clamp(localY * sy, 0.0f, static_cast<float>(vh)) : y;

                if (action == 0) {
                    g_touchCapturedByOverlay.store(false, std::memory_order_relaxed);
                    g_overlayPointerId.store(-1, std::memory_order_relaxed);
                }

                if (action == 0 || action == 5) {
                    const bool inWm = isPointInWatermark(scaledX, scaledY);
                    const bool inMenu = g_menuOpen.load(std::memory_order_relaxed) && isPointInMenu(scaledX, scaledY);
                    const bool captured = inWm || inMenu || g_imguiWantsMouse.load(std::memory_order_relaxed);
                    if (captured) {
                        g_touchCapturedByOverlay.store(true, std::memory_order_relaxed);
                        if (g_motionGetPointerId) {
                            const jint pid = env->CallIntMethod(inputEvent, g_motionGetPointerId, pointerIndex);
                            if (!clearJniException(env, "getting pointer id")) {
                                g_overlayPointerId.store(pid, std::memory_order_relaxed);
                            }
                        }
                    } else if (action == 0) {
                        g_touchCapturedByOverlay.store(false, std::memory_order_relaxed);
                        g_overlayPointerId.store(-1, std::memory_order_relaxed);
                    }
                }

                if (g_touchCapturedByOverlay.load(std::memory_order_relaxed) ||
                    g_imguiWantsMouse.load(std::memory_order_relaxed) ||
                    !g_menuOpen.load(std::memory_order_relaxed)) {
                    queuePointerEvent(scaledX, scaledY, kind);
                }

                if (action == 0) {
                    if (eventNumber <= 16) LOGI("Java touch down: x=%.1f y=%.1f display=%d", x, y, displayId);
                    handleTouch(scaledX, scaledY);
                }

                if (action == 0 || action == 5) {
                    shouldBlockEvent = g_touchCapturedByOverlay.load(std::memory_order_relaxed);
                } else if (action == 2) {
                    shouldBlockEvent = g_touchCapturedByOverlay.load(std::memory_order_relaxed) ||
                                       (g_menuOpen.load(std::memory_order_relaxed) && g_imguiWantsMouse.load(std::memory_order_relaxed));
                } else if (action == 1 || action == 3 || action == 6) {
                    shouldBlockEvent = g_touchCapturedByOverlay.load(std::memory_order_relaxed);
                    if (action == 1 || action == 3) {
                        g_touchCapturedByOverlay.store(false, std::memory_order_relaxed);
                        g_overlayPointerId.store(-1, std::memory_order_relaxed);
                    } else if (action == 6 && g_motionGetPointerId) {
                        const jint pid = env->CallIntMethod(inputEvent, g_motionGetPointerId, pointerIndex);
                        if (!clearJniException(env, "getting released pointer id")) {
                            if (pid == g_overlayPointerId.load(std::memory_order_relaxed) ||
                                g_overlayPointerId.load(std::memory_order_relaxed) == -1) {
                                g_touchCapturedByOverlay.store(false, std::memory_order_relaxed);
                                g_overlayPointerId.store(-1, std::memory_order_relaxed);
                            }
                        } else {
                            g_touchCapturedByOverlay.store(false, std::memory_order_relaxed);
                            g_overlayPointerId.store(-1, std::memory_order_relaxed);
                        }
                    }
                }
            }
        }
    }
    if (shouldBlockEvent) {
        return JNI_TRUE;
    }
    return g_realNativeInjectEvent
        ? g_realNativeInjectEvent(env, instance, inputEvent, displayId)
        : JNI_FALSE;
}

[[maybe_unused]] bool installJavaInputHook() {
    if (g_javaInputHookReady.load(std::memory_order_acquire)) return true;
#if !defined(__aarch64__)
    return false;
#else
    constexpr uintptr_t kNativeInjectEventRva = lemming::offsets::unity::kNativeInjectEvent;
    const uintptr_t unityBase = g_unityBase.load(std::memory_order_acquire);
    if (!unityBase) return false;

    JavaVM* vm = getOrInitJavaVm();
    if (!vm) return false;

    JNIEnv* env = nullptr;
    bool attachedHere = false;
    const jint envStatus = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (envStatus == JNI_EDETACHED) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) return false;
        attachedHere = true;
    } else if (envStatus != JNI_OK || !env) {
        return false;
    }

    bool installed = false;
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    if (!activityThread || clearJniException(env, "finding ActivityThread")) goto cleanup;
    {
        jmethodID currentApplication = env->GetStaticMethodID(
            activityThread, "currentApplication", "()Landroid/app/Application;");
        if (!currentApplication || clearJniException(env, "finding currentApplication")) goto cleanup;
        jobject application = env->CallStaticObjectMethod(activityThread, currentApplication);
        if (!application || clearJniException(env, "getting current Application")) goto cleanup;

        jclass applicationClass = env->GetObjectClass(application);
        jmethodID getClassLoader = applicationClass
            ? env->GetMethodID(applicationClass, "getClassLoader", "()Ljava/lang/ClassLoader;")
            : nullptr;
        if (!getClassLoader || clearJniException(env, "finding getClassLoader")) {
            if (applicationClass) env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }
        jobject classLoader = env->CallObjectMethod(application, getClassLoader);
        if (!classLoader || clearJniException(env, "getting app ClassLoader")) {
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
        jmethodID loadClass = classLoaderClass
            ? env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;")
            : nullptr;
        jstring unityName = env->NewStringUTF("com.unity3d.player.UnityPlayer");
        jobject unityClassObject = (loadClass && unityName)
            ? env->CallObjectMethod(classLoader, loadClass, unityName)
            : nullptr;
        if (unityClassObject) {
            jfieldID currentActivityField = env->GetStaticFieldID(
                reinterpret_cast<jclass>(unityClassObject), "currentActivity", "Landroid/app/Activity;");
            if (currentActivityField && !clearJniException(env, "finding UnityPlayer.currentActivity")) {
                jobject act = env->GetStaticObjectField(
                    reinterpret_cast<jclass>(unityClassObject), currentActivityField);
                if (act && !clearJniException(env, "getting UnityPlayer.currentActivity")) {
                    if (!g_appActivity) {
                        g_appActivity = env->NewGlobalRef(act);
                    }
                    setupImeBridge(env, act);
                    env->DeleteLocalRef(act);
                }
            }
        }
        if (!unityClassObject || clearJniException(env, "loading UnityPlayer class")) {
            if (unityName) env->DeleteLocalRef(unityName);
            if (classLoaderClass) env->DeleteLocalRef(classLoaderClass);
            env->DeleteLocalRef(classLoader);
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        jclass motionEventLocal = env->FindClass("android/view/MotionEvent");
        jclass motionGlobal = motionEventLocal
            ? reinterpret_cast<jclass>(env->NewGlobalRef(motionEventLocal))
            : nullptr;
        if (!motionGlobal || clearJniException(env, "preparing MotionEvent")) {
            if (motionEventLocal) env->DeleteLocalRef(motionEventLocal);
            env->DeleteLocalRef(unityClassObject);
            env->DeleteLocalRef(unityName);
            env->DeleteLocalRef(classLoaderClass);
            env->DeleteLocalRef(classLoader);
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        jmethodID getActionMasked = env->GetMethodID(motionGlobal, "getActionMasked", "()I");
        jmethodID getActionIndex = env->GetMethodID(motionGlobal, "getActionIndex", "()I");
        jmethodID getX = env->GetMethodID(motionGlobal, "getX", "(I)F");
        jmethodID getY = env->GetMethodID(motionGlobal, "getY", "(I)F");
        jmethodID getPointerId = env->GetMethodID(motionGlobal, "getPointerId", "(I)I");
        jmethodID findPointerIndex = env->GetMethodID(motionGlobal, "findPointerIndex", "(I)I");
        if (getPointerId) g_motionGetPointerId = getPointerId;
        if (findPointerIndex) g_motionFindPointerIndex = findPointerIndex;
        if (!getActionMasked || !getActionIndex || !getX || !getY ||
            clearJniException(env, "resolving MotionEvent methods")) {
            env->DeleteGlobalRef(motionGlobal);
            env->DeleteLocalRef(motionEventLocal);
            env->DeleteLocalRef(unityClassObject);
            env->DeleteLocalRef(unityName);
            env->DeleteLocalRef(classLoaderClass);
            env->DeleteLocalRef(classLoader);
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        // Resolve View getWidth / getHeight / getLocationOnScreen
        jclass viewLocal = env->FindClass("android/view/View");
        if (viewLocal && !clearJniException(env, "finding View class")) {
            g_viewGetWidth = env->GetMethodID(viewLocal, "getWidth", "()I");
            g_viewGetHeight = env->GetMethodID(viewLocal, "getHeight", "()I");
            g_viewGetLocationOnScreen = env->GetMethodID(viewLocal, "getLocationOnScreen", "([I)V");
            clearJniException(env, "resolving View methods");
            env->DeleteLocalRef(viewLocal);
        }

        // Query initial display metrics from Application resources
        jmethodID getResources = env->GetMethodID(
            applicationClass, "getResources", "()Landroid/content/res/Resources;");
        if (getResources && !clearJniException(env, "finding getResources")) {
            jobject resources = env->CallObjectMethod(application, getResources);
            if (resources && !clearJniException(env, "calling getResources")) {
                jclass resClass = env->GetObjectClass(resources);
                jmethodID getDisplayMetrics = resClass
                    ? env->GetMethodID(resClass, "getDisplayMetrics", "()Landroid/util/DisplayMetrics;")
                    : nullptr;
                if (getDisplayMetrics && !clearJniException(env, "finding getDisplayMetrics")) {
                    jobject metrics = env->CallObjectMethod(resources, getDisplayMetrics);
                    if (metrics && !clearJniException(env, "calling getDisplayMetrics")) {
                        jclass dmClass = env->GetObjectClass(metrics);
                        jfieldID widthPixels = dmClass ? env->GetFieldID(dmClass, "widthPixels", "I") : nullptr;
                        jfieldID heightPixels = dmClass ? env->GetFieldID(dmClass, "heightPixels", "I") : nullptr;
                        if (widthPixels && heightPixels && !clearJniException(env, "finding DisplayMetrics fields")) {
                            const jint w = env->GetIntField(metrics, widthPixels);
                            const jint h = env->GetIntField(metrics, heightPixels);
                            if (w > 0 && h > 0) {
                                g_viewWidth.store(w, std::memory_order_relaxed);
                                g_viewHeight.store(h, std::memory_order_relaxed);
                                LOGI("Initial display metrics from Java: %dx%d", w, h);
                            }
                        }
                        if (dmClass) env->DeleteLocalRef(dmClass);
                        env->DeleteLocalRef(metrics);
                    }
                }
                if (resClass) env->DeleteLocalRef(resClass);
                env->DeleteLocalRef(resources);
            }
        }

        // Query physical screen size via WindowManager (API 30+ getCurrentWindowMetrics or getRealMetrics)
        int physW = 0, physH = 0;
        jstring windowService = env->NewStringUTF("window");
        jmethodID getSystemService = env->GetMethodID(
            applicationClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
        jobject windowManager = (getSystemService && windowService)
            ? env->CallObjectMethod(application, getSystemService, windowService)
            : nullptr;
        if (windowService) env->DeleteLocalRef(windowService);

        if (windowManager && !clearJniException(env, "getting WindowManager")) {
            jclass wmClass = env->GetObjectClass(windowManager);

            // 1. Try API 30+ getCurrentWindowMetrics() -> getBounds()
            jmethodID getCurrentWindowMetrics = wmClass
                ? env->GetMethodID(wmClass, "getCurrentWindowMetrics", "()Landroid/view/WindowMetrics;")
                : nullptr;
            if (getCurrentWindowMetrics && !clearJniException(env, "finding getCurrentWindowMetrics")) {
                jobject windowMetrics = env->CallObjectMethod(windowManager, getCurrentWindowMetrics);
                if (windowMetrics && !clearJniException(env, "calling getCurrentWindowMetrics")) {
                    jclass wmMetricsClass = env->GetObjectClass(windowMetrics);
                    jmethodID getBounds = wmMetricsClass
                        ? env->GetMethodID(wmMetricsClass, "getBounds", "()Landroid/graphics/Rect;")
                        : nullptr;
                    if (getBounds && !clearJniException(env, "finding getBounds")) {
                        jobject bounds = env->CallObjectMethod(windowMetrics, getBounds);
                        if (bounds && !clearJniException(env, "calling getBounds")) {
                            jclass rectClass = env->GetObjectClass(bounds);
                            jmethodID widthMethod = rectClass ? env->GetMethodID(rectClass, "width", "()I") : nullptr;
                            jmethodID heightMethod = rectClass ? env->GetMethodID(rectClass, "height", "()I") : nullptr;
                            if (widthMethod && heightMethod && !clearJniException(env, "finding Rect dimensions")) {
                                physW = env->CallIntMethod(bounds, widthMethod);
                                physH = env->CallIntMethod(bounds, heightMethod);
                                clearJniException(env, "calling Rect dimensions");
                            }
                            if (rectClass) env->DeleteLocalRef(rectClass);
                            env->DeleteLocalRef(bounds);
                        }
                    }
                    if (wmMetricsClass) env->DeleteLocalRef(wmMetricsClass);
                    env->DeleteLocalRef(windowMetrics);
                }
            }

            // 2. Fallback to getDefaultDisplay().getRealMetrics()
            if (physW <= 0 || physH <= 0) {
                jmethodID getDefaultDisplay = wmClass
                    ? env->GetMethodID(wmClass, "getDefaultDisplay", "()Landroid/view/Display;")
                    : nullptr;
                if (getDefaultDisplay && !clearJniException(env, "finding getDefaultDisplay")) {
                    jobject display = env->CallObjectMethod(windowManager, getDefaultDisplay);
                    if (display && !clearJniException(env, "calling getDefaultDisplay")) {
                        jclass displayClass = env->GetObjectClass(display);
                        jclass dmClass = env->FindClass("android/util/DisplayMetrics");
                        jmethodID dmInit = dmClass ? env->GetMethodID(dmClass, "<init>", "()V") : nullptr;
                        jmethodID getRealMetrics = (displayClass && dmClass && dmInit)
                            ? env->GetMethodID(displayClass, "getRealMetrics", "(Landroid/util/DisplayMetrics;)V")
                            : nullptr;
                        if (getRealMetrics && !clearJniException(env, "finding getRealMetrics")) {
                            jobject realMetrics = env->NewObject(dmClass, dmInit);
                            if (realMetrics && !clearJniException(env, "creating DisplayMetrics")) {
                                env->CallVoidMethod(display, getRealMetrics, realMetrics);
                                if (!clearJniException(env, "calling getRealMetrics")) {
                                    jfieldID widthPixels = env->GetFieldID(dmClass, "widthPixels", "I");
                                    jfieldID heightPixels = env->GetFieldID(dmClass, "heightPixels", "I");
                                    if (widthPixels && heightPixels && !clearJniException(env, "getting real DisplayMetrics fields")) {
                                        physW = env->GetIntField(realMetrics, widthPixels);
                                        physH = env->GetIntField(realMetrics, heightPixels);
                                    }
                                }
                                env->DeleteLocalRef(realMetrics);
                            }
                        }
                        if (dmClass) env->DeleteLocalRef(dmClass);
                        if (displayClass) env->DeleteLocalRef(displayClass);
                        env->DeleteLocalRef(display);
                    }
                }
            }
            if (wmClass) env->DeleteLocalRef(wmClass);
            env->DeleteLocalRef(windowManager);
        }

        if (physW > 0 && physH > 0) {
            if (physW < physH) std::swap(physW, physH);
            g_physicalWidth.store(physW, std::memory_order_relaxed);
            g_physicalHeight.store(physH, std::memory_order_relaxed);
            LOGI("Physical screen size detected via JNI WindowManager: %dx%d", physW, physH);
        }

        g_realNativeInjectEvent = reinterpret_cast<NativeInjectEventFn>(
            unityBase + kNativeInjectEventRva);
        JNINativeMethod method{
            const_cast<char*>("nativeInjectEvent"),
            const_cast<char*>("(Landroid/view/InputEvent;I)Z"),
            reinterpret_cast<void*>(hookedNativeInjectEvent)
        };
        const jint result = env->RegisterNatives(
            reinterpret_cast<jclass>(unityClassObject), &method, 1);
        if (result == JNI_OK && !clearJniException(env, "registering nativeInjectEvent")) {
            g_motionEventClass = motionGlobal;
            g_motionGetActionMasked = getActionMasked;
            g_motionGetActionIndex = getActionIndex;
            g_motionGetX = getX;
            g_motionGetY = getY;
            g_javaInputHookReady.store(true, std::memory_order_release);
            installed = true;
            LOGI("Java input hook ready: nativeInjectEvent RVA=0x%llx",
                 static_cast<unsigned long long>(kNativeInjectEventRva));
        } else {
            env->DeleteGlobalRef(motionGlobal);
            clearJniException(env, "RegisterNatives failure");
        }

        env->DeleteLocalRef(motionEventLocal);
        env->DeleteLocalRef(unityClassObject);
        env->DeleteLocalRef(unityName);
        env->DeleteLocalRef(classLoaderClass);
        env->DeleteLocalRef(classLoader);
        env->DeleteLocalRef(applicationClass);
        env->DeleteLocalRef(application);
    }
cleanup:
    if (activityThread) env->DeleteLocalRef(activityThread);
    if (attachedHere) vm->DetachCurrentThread();
    return installed;
#endif
}

void renderBeforePresent(EGLDisplay display, EGLSurface surface, const char* path) {
    const auto frame = g_presentFrames.fetch_add(1, std::memory_order_relaxed) + 1;
    EGLint width = 0;
    EGLint height = 0;
    const EGLContext context = eglGetCurrentContext();
    if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT ||
        eglQuerySurface(display, surface, EGL_WIDTH, &width) != EGL_TRUE ||
        eglQuerySurface(display, surface, EGL_HEIGHT, &height) != EGL_TRUE || width <= 0 || height <= 0) {
        g_renderFailures.fetch_add(1, std::memory_order_relaxed);
        if (frame <= 5 || frame % 300 == 0) {
            LOGW("present skipped: path=%s display=%p surface=%p context=%p size=%dx%d eglError=0x%x",
                 path, display, surface, context, width, height, static_cast<unsigned>(eglGetError()));
        }
        return;
    }

    g_screenWidth.store(width, std::memory_order_relaxed);
    g_screenHeight.store(height, std::memory_order_relaxed);
    drawOverlay(display, surface, width, height);

    if (frame <= 3 || frame % 300 == 0) {
        const int vw = g_viewWidth.load(std::memory_order_relaxed);
        const int vh = g_viewHeight.load(std::memory_order_relaxed);
        LOGI("render heartbeat: path=%s frame=%llu buffer=%dx%d view=%dx%d scale=%.3fx%.3f context=%p menu=%d esp=%d status=%s",
             path, static_cast<unsigned long long>(frame), width, height,
             vw, vh,
             vw > 0 ? static_cast<float>(width) / static_cast<float>(vw) : 1.0f,
             vh > 0 ? static_cast<float>(height) / static_cast<float>(vh) : 1.0f,
             context,
             g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0,
             g_espBoxesEnabled.load(std::memory_order_relaxed) ? 1 : 0,
             lemming::esp::statusText());
    }
}

EGLBoolean hookedSwapBuffers(EGLDisplay display, EGLSurface surface) {
    renderBeforePresent(display, surface, "eglSwapBuffers");
    return g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE;
}

EGLBoolean hookedSwapBuffersWithDamageKHR(EGLDisplay display, EGLSurface surface,
                                          const EGLint* rects, EGLint count) {
    renderBeforePresent(display, surface, "eglSwapBuffersWithDamageKHR");
    return g_realSwapBuffersWithDamageKHR
        ? g_realSwapBuffersWithDamageKHR(display, surface, rects, count)
        : (g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE);
}

EGLBoolean hookedSwapBuffersWithDamageEXT(EGLDisplay display, EGLSurface surface,
                                          const EGLint* rects, EGLint count) {
    renderBeforePresent(display, surface, "eglSwapBuffersWithDamageEXT");
    return g_realSwapBuffersWithDamageEXT
        ? g_realSwapBuffersWithDamageEXT(display, surface, rects, count)
        : (g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE);
}

int32_t hookedInputGetEvent(AInputQueue* queue, AInputEvent** outEvent) {
    if (!g_realInputGetEvent) return -1;
    const int32_t result = g_realInputGetEvent(queue, outEvent);
    if (result >= 0 && outEvent && *outEvent && AInputEvent_getType(*outEvent) == AINPUT_EVENT_TYPE_MOTION) {
        const int32_t action = AMotionEvent_getAction(*outEvent);
        const int32_t masked = action & AMOTION_EVENT_ACTION_MASK;
        const size_t actionIndex = static_cast<size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                                       AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        const size_t count = AMotionEvent_getPointerCount(*outEvent);
        const size_t index = masked == AMOTION_EVENT_ACTION_MOVE ? 0 : actionIndex;
        if (index < count) {
            const float x = AMotionEvent_getX(*outEvent, index);
            const float y = AMotionEvent_getY(*outEvent, index);
            int kind = 0;
            if (masked == AMOTION_EVENT_ACTION_DOWN || masked == AMOTION_EVENT_ACTION_POINTER_DOWN) kind = 1;
            if (masked == AMOTION_EVENT_ACTION_UP || masked == AMOTION_EVENT_ACTION_POINTER_UP ||
                masked == AMOTION_EVENT_ACTION_CANCEL) kind = 2;

            int pw = g_physicalWidth.load(std::memory_order_relaxed);
            int ph = g_physicalHeight.load(std::memory_order_relaxed);
            if (pw <= 0 || ph <= 0) {
                pw = g_screenWidth.load(std::memory_order_relaxed);
                ph = g_screenHeight.load(std::memory_order_relaxed);
                if (pw < ph) std::swap(pw, ph);
            }
            const int vw = g_viewWidth.load(std::memory_order_relaxed);
            const int vh = g_viewHeight.load(std::memory_order_relaxed);
            const int offX = g_viewOffsetX.load(std::memory_order_relaxed);
            const int offY = g_viewOffsetY.load(std::memory_order_relaxed);
            const float localX = x - static_cast<float>(offX);
            const float localY = y - static_cast<float>(offY);
            const float sx = (pw > 0 && vw > 0) ? (static_cast<float>(vw) / static_cast<float>(pw)) : 1.0f;
            const float sy = (ph > 0 && vh > 0) ? (static_cast<float>(vh) / static_cast<float>(ph)) : 1.0f;
            const float scaledX = (vw > 0) ? std::clamp(localX * sx, 0.0f, static_cast<float>(vw)) : x;
            const float scaledY = (vh > 0) ? std::clamp(localY * sy, 0.0f, static_cast<float>(vh)) : y;

            queuePointerEvent(scaledX, scaledY, kind);
            if (kind == 1) handleTouch(scaledX, scaledY);
        }
    }
    return result;
}

uintptr_t makeAbsolute(uintptr_t base, uintptr_t value, uintptr_t minAddress, uintptr_t maxAddress) {
    if (value >= minAddress && value < maxAddress) return value;
    return base + value;
}

int protectionForAddress(const void* address) {
    FILE* file = std::fopen("/proc/self/maps", "r");
    if (!file) return PROT_READ;
    char line[512];
    const auto target = reinterpret_cast<uintptr_t>(address);
    int result = PROT_READ;
    while (std::fgets(line, sizeof(line), file)) {
        unsigned long long begin = 0;
        unsigned long long end = 0;
        char perms[5]{};
        if (std::sscanf(line, "%llx-%llx %4s", &begin, &end, perms) != 3) continue;
        if (target < begin || target >= end) continue;
        result = 0;
        if (perms[0] == 'r') result |= PROT_READ;
        if (perms[1] == 'w') result |= PROT_WRITE;
        if (perms[2] == 'x') result |= PROT_EXEC;
        break;
    }
    std::fclose(file);
    return result;
}

bool patchSlot(void** slot, void* replacement, void** original) {
    if (!slot || !replacement) return false;
    void* current = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    if (current == replacement) return false;
    if (original && !*original && current) *original = current;

    const long pageSizeLong = sysconf(_SC_PAGESIZE);
    if (pageSizeLong <= 0) return false;
    const auto pageSize = static_cast<size_t>(pageSizeLong);
    const uintptr_t page = reinterpret_cast<uintptr_t>(slot) & ~(static_cast<uintptr_t>(pageSize) - 1u);
    const int originalProtection = protectionForAddress(slot);
    if (mprotect(reinterpret_cast<void*>(page), pageSize, originalProtection | PROT_WRITE) != 0) {
        LOGE("mprotect RW failed for GOT slot=%p: errno=%d %s", slot, errno, std::strerror(errno));
        return false;
    }
    __atomic_store_n(slot, replacement, __ATOMIC_RELEASE);
    __builtin___clear_cache(reinterpret_cast<char*>(slot), reinterpret_cast<char*>(slot) + sizeof(void*));
    if (mprotect(reinterpret_cast<void*>(page), pageSize, originalProtection) != 0) {
        LOGW("mprotect restore failed for GOT slot=%p: errno=%d %s", slot, errno, std::strerror(errno));
    }
    return true;
}

struct HookTargets {
    const char* name;
    void* replacement;
    void** original;
    std::atomic<int>* slotCount;
};

template <typename Relocation>
bool patchRelocations(uintptr_t base,
                      const Relocation* relocs,
                      size_t count,
                      const ElfW(Sym)* symbols,
                      const char* strings,
                      const HookTargets* targets,
                      size_t targetCount) {
    bool changed = false;
    for (size_t i = 0; i < count; ++i) {
        const Relocation& rel = relocs[i];
        const unsigned type = LR_R_TYPE(rel.r_info);
        if (type != LR_JUMP_SLOT && type != LR_GLOB_DAT) continue;
        const unsigned symIndex = LR_R_SYM(rel.r_info);
        const char* symbolName = strings + symbols[symIndex].st_name;
        for (size_t t = 0; t < targetCount; ++t) {
            if (std::strcmp(symbolName, targets[t].name) != 0) continue;
            auto** slot = reinterpret_cast<void**>(base + static_cast<uintptr_t>(rel.r_offset));
            if (patchSlot(slot, targets[t].replacement, targets[t].original)) {
                targets[t].slotCount->fetch_add(1, std::memory_order_relaxed);
                changed = true;
            }
        }
    }
    return changed;
}

int hookModule(dl_phdr_info* info, size_t, void*) {
    const char* moduleName = info->dlpi_name ? info->dlpi_name : "";
    if (!std::strstr(moduleName, "libunity.so") &&
        !std::strstr(moduleName, "libil2cpp.so") &&
        !std::strstr(moduleName, "libmain.so") &&
        !std::strstr(moduleName, "libGfxDevice")) {
        return 0;
    }

    const uintptr_t base = static_cast<uintptr_t>(info->dlpi_addr);
    uintptr_t minAddress = UINTPTR_MAX;
    uintptr_t maxAddress = 0;
    const ElfW(Dyn)* dynamic = nullptr;

    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)& ph = info->dlpi_phdr[i];
        if (ph.p_type == PT_LOAD) {
            minAddress = std::min(minAddress, base + static_cast<uintptr_t>(ph.p_vaddr));
            maxAddress = std::max(maxAddress, base + static_cast<uintptr_t>(ph.p_vaddr + ph.p_memsz));
        } else if (ph.p_type == PT_DYNAMIC) {
            dynamic = reinterpret_cast<const ElfW(Dyn)*>(base + ph.p_vaddr);
        }
    }
    if (!dynamic || minAddress == UINTPTR_MAX) return 0;

    if (std::strstr(moduleName, "libunity.so")) {
        g_unityBase.store(base, std::memory_order_release);
#if defined(__aarch64__)
        const uintptr_t codeProbe = base + lemming::offsets::unity::kCameraWorldToScreen;
        const uintptr_t typeProbe = base + lemming::offsets::typeinfo::kPlayer;
        const bool ready = codeProbe >= minAddress && codeProbe < maxAddress &&
                           typeProbe >= minAddress && typeProbe < maxAddress;
        g_offsetProfileReady.store(ready, std::memory_order_release);
#else
        g_offsetProfileReady.store(false, std::memory_order_release);
#endif
    }

    const ElfW(Sym)* symbols = nullptr;
    const char* strings = nullptr;
    const void* pltRelocations = nullptr;
    const ElfW(Rel)* regularRel = nullptr;
    const ElfW(Rela)* regularRela = nullptr;
    uintptr_t pltRelType = 0;
    size_t pltRelSize = 0;
    size_t regularRelSize = 0;
    size_t regularRelaSize = 0;

    for (const ElfW(Dyn)* d = dynamic; d->d_tag != DT_NULL; ++d) {
        const uintptr_t value = static_cast<uintptr_t>(d->d_un.d_ptr);
        switch (d->d_tag) {
            case DT_SYMTAB:
                symbols = reinterpret_cast<const ElfW(Sym)*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_STRTAB:
                strings = reinterpret_cast<const char*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_JMPREL:
                pltRelocations = reinterpret_cast<const void*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_PLTRELSZ:
                pltRelSize = static_cast<size_t>(d->d_un.d_val);
                break;
            case DT_PLTREL:
                pltRelType = static_cast<uintptr_t>(d->d_un.d_val);
                break;
            case DT_REL:
                regularRel = reinterpret_cast<const ElfW(Rel)*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_RELSZ:
                regularRelSize = static_cast<size_t>(d->d_un.d_val);
                break;
            case DT_RELA:
                regularRela = reinterpret_cast<const ElfW(Rela)*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_RELASZ:
                regularRelaSize = static_cast<size_t>(d->d_un.d_val);
                break;
            default:
                break;
        }
    }
    if (!symbols || !strings) return 0;

    HookTargets targets[] = {
        {"eglSwapBuffers", reinterpret_cast<void*>(hookedSwapBuffers),
         reinterpret_cast<void**>(&g_realSwapBuffers), &g_swapSlots},
        {"eglSwapBuffersWithDamageKHR", reinterpret_cast<void*>(hookedSwapBuffersWithDamageKHR),
         reinterpret_cast<void**>(&g_realSwapBuffersWithDamageKHR), &g_swapDamageSlots},
        {"eglSwapBuffersWithDamageEXT", reinterpret_cast<void*>(hookedSwapBuffersWithDamageEXT),
         reinterpret_cast<void**>(&g_realSwapBuffersWithDamageEXT), &g_swapDamageSlots},
        {"AInputQueue_getEvent", reinterpret_cast<void*>(hookedInputGetEvent),
         reinterpret_cast<void**>(&g_realInputGetEvent), &g_inputSlots},
    };
    const size_t targetCount = sizeof(targets) / sizeof(targets[0]);

    if (pltRelocations && pltRelSize) {
        if (pltRelType == DT_REL) {
            const auto* relocs = reinterpret_cast<const ElfW(Rel)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, targetCount);
        } else if (pltRelType == DT_RELA) {
            const auto* relocs = reinterpret_cast<const ElfW(Rela)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rela)), symbols, strings, targets, targetCount);
        }
    }
    if (regularRel && regularRelSize) {
        patchRelocations(base, regularRel, regularRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, targetCount);
    }
    if (regularRela && regularRelaSize) {
        patchRelocations(base, regularRela, regularRelaSize / sizeof(ElfW(Rela)), symbols, strings, targets, targetCount);
    }
    return 0;
}

void resolveOriginals() {
    void* egl = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (egl) {
        g_realSwapBuffers = reinterpret_cast<SwapBuffersFn>(dlsym(egl, "eglSwapBuffers"));
        g_realSwapBuffersWithDamageKHR = reinterpret_cast<SwapBuffersWithDamageFn>(
            dlsym(egl, "eglSwapBuffersWithDamageKHR"));
        g_realSwapBuffersWithDamageEXT = reinterpret_cast<SwapBuffersWithDamageFn>(
            dlsym(egl, "eglSwapBuffersWithDamageEXT"));
    }
    if (!g_realSwapBuffersWithDamageKHR) {
        g_realSwapBuffersWithDamageKHR = reinterpret_cast<SwapBuffersWithDamageFn>(
            eglGetProcAddress("eglSwapBuffersWithDamageKHR"));
    }
    if (!g_realSwapBuffersWithDamageEXT) {
        g_realSwapBuffersWithDamageEXT = reinterpret_cast<SwapBuffersWithDamageFn>(
            eglGetProcAddress("eglSwapBuffersWithDamageEXT"));
    }
    void* android = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (android) g_realInputGetEvent = reinterpret_cast<InputGetEventFn>(dlsym(android, "AInputQueue_getEvent"));
    LOGI("resolved originals: swap=%p damageKHR=%p damageEXT=%p input=%p",
         reinterpret_cast<void*>(g_realSwapBuffers),
         reinterpret_cast<void*>(g_realSwapBuffersWithDamageKHR),
         reinterpret_cast<void*>(g_realSwapBuffersWithDamageEXT),
         reinterpret_cast<void*>(g_realInputGetEvent));
}

void handleToggleSignal(int) {
    g_toggleMenuSignal = 1;
}

void handleDumpSignal(int) {
    g_dumpDiagnosticsSignal = 1;
}

void logDiagnostics(const char* reason) {
    LOGI("diagnostics[%s]: profile=%s ready=%d base=%p hooks(swap=%d damage=%d input=%d java=%d) frames(present=%llu rendered=%llu failures=%llu) EGL(context=%p surface=%p GLES=%d) menu=%d esp=%d",
         reason ? reason : "manual", lemming::offsets::kProfileName,
         g_offsetProfileReady.load(std::memory_order_relaxed) ? 1 : 0,
         reinterpret_cast<void*>(g_unityBase.load(std::memory_order_relaxed)),
         g_swapSlots.load(std::memory_order_relaxed),
         g_swapDamageSlots.load(std::memory_order_relaxed),
         g_inputSlots.load(std::memory_order_relaxed),
         g_javaInputHookReady.load(std::memory_order_relaxed) ? 1 : 0,
         static_cast<unsigned long long>(g_presentFrames.load(std::memory_order_relaxed)),
         static_cast<unsigned long long>(g_renderedFrames.load(std::memory_order_relaxed)),
         static_cast<unsigned long long>(g_renderFailures.load(std::memory_order_relaxed)),
         reinterpret_cast<void*>(g_lastEglContext.load(std::memory_order_relaxed)),
         reinterpret_cast<void*>(g_lastEglSurface.load(std::memory_order_relaxed)),
         g_glClientVersion.load(std::memory_order_relaxed),
         g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0,
         g_espBoxesEnabled.load(std::memory_order_relaxed) ? 1 : 0);
    lemming::esp::logDiagnostics(reason);
    LOGI("AIM diagnostics[%s]: normalHook=%d silentHook=%d normalState=%s silentState=%s angleUpdates=%llu rayUpdates=%llu", reason, lemming::aim::g_normalHookReady.load()?1:0, lemming::aim::g_silentHookReady.load()?1:0, lemming::aim::stateText(lemming::aim::g_normalState.load()), lemming::aim::stateText(lemming::aim::g_silentState.load()), static_cast<unsigned long long>(lemming::aim::g_normalApplications.load()), static_cast<unsigned long long>(lemming::aim::g_silentApplications.load()));
}

void hookWorker() {
    lemming::esp::detail::g_playerFrameCallback.store(&lemming::aim::runtime::onPlayer, std::memory_order_release);
    resolveOriginals();
    int lastSwap = -1;
    int lastDamage = -1;
    int lastInput = -1;
    uintptr_t lastUnityBase = 0;
    bool lastProfileReady = false;
    std::uint64_t lastPresent = 0;
    int stagnantChecks = 0;
    auto lastAimDebug = std::chrono::steady_clock::time_point{};
    for (;;) {
        if (g_toggleMenuSignal) {
            g_toggleMenuSignal = 0;
            LOGI("SIGUSR1 ignored: menu toggle is watermark-only");
        }
        if (g_dumpDiagnosticsSignal) {
            g_dumpDiagnosticsSignal = 0;
            logDiagnostics("SIGUSR2");
        }

        dl_iterate_phdr(hookModule, nullptr);
#if defined(__aarch64__)
        if (!g_javaInputHookReady.load(std::memory_order_acquire)) installJavaInputHook();
#endif

        const int swaps = g_swapSlots.load(std::memory_order_relaxed);
        const int damage = g_swapDamageSlots.load(std::memory_order_relaxed);
        const int inputs = g_inputSlots.load(std::memory_order_relaxed);
        if (swaps != lastSwap || damage != lastDamage || inputs != lastInput) {
            LOGI("hook status: swap=%d damage=%d nativeInput=%d javaInput=%d signals=USR1/USR2",
                 swaps, damage, inputs, g_javaInputHookReady.load(std::memory_order_relaxed) ? 1 : 0);
            lastSwap = swaps;
            lastDamage = damage;
            lastInput = inputs;
        }

        const uintptr_t unityBase = g_unityBase.load(std::memory_order_acquire);
        const bool profileReady = g_offsetProfileReady.load(std::memory_order_acquire);
        if (unityBase != lastUnityBase || profileReady != lastProfileReady) {
            LOGI("ARM64 profile: base=%p ready=%d profile=%s source=%s",
                 reinterpret_cast<void*>(unityBase), profileReady ? 1 : 0,
                 lemming::offsets::kProfileName, lemming::offsets::kSourceSha256);
            lastUnityBase = unityBase;
            lastProfileReady = profileReady;
        }

        const std::uint64_t presents = g_presentFrames.load(std::memory_order_relaxed);
        if (presents == lastPresent) {
            ++stagnantChecks;
            if (stagnantChecks == 5) {
                LOGW("no intercepted EGL present calls for 10s: swapSlots=%d damageSlots=%d; check injection timing or Vulkan renderer",
                     swaps, damage);
                logDiagnostics("no-present-watchdog");
                stagnantChecks = 0;
            }
        } else {
            stagnantChecks = 0;
            lastPresent = presents;
        }
        auto aimNow = std::chrono::steady_clock::now();
        if (aimNow - lastAimDebug >= std::chrono::seconds(5)) { lastAimDebug = aimNow; lemming::aim::debug::report(); }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void start() {
    bool expected = false;
    if (!g_started.compare_exchange_strong(expected, true)) return;

    struct sigaction toggleAction{};
    toggleAction.sa_handler = handleToggleSignal;
    sigemptyset(&toggleAction.sa_mask);
    toggleAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &toggleAction, nullptr);

    struct sigaction dumpAction{};
    dumpAction.sa_handler = handleDumpSignal;
    sigemptyset(&dumpAction.sa_mask);
    dumpAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &dumpAction, nullptr);

    std::thread(hookWorker).detach();
    LOGI("bootstrap started: build=%s arch=%s SIGUSR1=toggle SIGUSR2=diagnostics",
         lemming::offsets::kProfileName, kArchitectureLabel);
}

} // namespace

extern "C" __attribute__((visibility("default"))) void LemmingRMT_ToggleMenu() {
    const bool next = !g_menuOpen.load(std::memory_order_relaxed);
    g_menuOpen.store(next, std::memory_order_relaxed);
    LOGI("menu toggled by export: open=%d", next ? 1 : 0);
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_SetMenuOpen(int open) {
    g_menuOpen.store(open != 0, std::memory_order_relaxed);
    LOGI("menu set by export: open=%d", open != 0 ? 1 : 0);
}

extern "C" __attribute__((visibility("default"))) int LemmingRMT_IsMenuOpen() {
    return g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0;
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_SetEspEnabled(int enabled) {
    g_espBoxesEnabled.store(enabled != 0, std::memory_order_relaxed);
    LOGI("basic ESP set by export: enabled=%d", enabled != 0 ? 1 : 0);
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_DumpDiagnostics() {
    logDiagnostics("export");
}

extern "C" __attribute__((visibility("default"))) uintptr_t LemmingRMT_GetUnityBase() {
    return g_unityBase.load(std::memory_order_acquire);
}

extern "C" __attribute__((visibility("default"))) uintptr_t LemmingRMT_ResolveUnityRva(uintptr_t rva) {
    const uintptr_t base = g_unityBase.load(std::memory_order_acquire);
    return base ? base + rva : 0;
}

extern "C" __attribute__((visibility("default"))) int LemmingRMT_IsOffsetProfileReady() {
    return g_offsetProfileReady.load(std::memory_order_acquire) ? 1 : 0;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    g_javaVm = vm;
    start();
    return JNI_VERSION_1_6;
}

__attribute__((constructor)) static void LemmingRMT_OnLoad() {
    start();
}
