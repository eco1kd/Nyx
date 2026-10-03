#pragma once
// Included inside main.cpp's anonymous namespace; all widgets stay on the render thread.
struct EspAppearance {
    bool box=true,health=true,armor=true,weapon=true,ammo=true,fill=false,rounded=true;
    int boxStyle=0;
    float lineWidth=1.5f,fillIntensity=0.08f,cornerRadius=6.0f;
    ImVec4 boxColor{0.69f,0.77f,1.0f,1.0f};
    ImVec4 healthColor{0.36f,0.87f,0.65f,1.0f},armorColor{0.43f,0.68f,1.0f,1.0f};
    ImVec4 textColor{0.91f,0.94f,1.0f,1.0f},accent{0.59f,0.68f,1.0f,1.0f};
};
EspAppearance g_appearance;
float g_uiScale=1.0f,g_menuAnimation=0.0f,g_pageAnimation=1.0f;
int g_section=0,g_subsection=0;
bool g_espEditorOpen=false;
static bool is_logged_in = false;
static char login_input[32] = "";
static char password_input[32] = "";
static bool show_login_error = false;
static bool g_watermark_enabled = true;
static float g_watermark_opacity = 0.85f;
static float g_watermark_accent[4] = {0.59f, 0.68f, 1.0f, 1.0f};

// Window and FPS settings
static float g_window_scale = 1.0f;
static bool g_window_locked = false;
static ImVec2 g_window_pos(0.0f, 0.0f);
static ImVec2 g_window_size(820.0f, 650.0f);
static bool g_reset_window_pos = false;
static bool g_show_fps = true;

// Misc settings
static bool g_misc_no_flash = false;
static bool g_misc_no_smoke = false;
static bool g_misc_third_person = false;
static float g_misc_third_person_dist = 120.0f;
static float g_misc_fov = 90.0f;
static bool g_misc_hit_sound = true;
static bool g_misc_bhop = false;
static bool g_misc_auto_strafe = false;
static bool g_misc_fast_crouch = false;
static bool g_misc_fast_switch = false;
static bool g_misc_quick_reload = false;
static bool g_misc_anti_screenshot = false;

// Visuals Chams & World
static bool g_chams_enemy = true;
static bool g_chams_team = false;
static bool g_chams_wireframe = false;
static bool g_world_nightmode = false;
static float g_world_brightness = 1.0f;
static bool g_world_remove_fog = true;

// Config presets
static char config_name[32] = "default";
static int g_selected_preset = 0;

// Settings
static float g_user_ui_scale = 1.0f;
static int g_selected_language = 0;

float uiEase(float value,float target,float speed=18.0f){return value+(target-value)*(1.0f-std::exp(-speed*std::min(ImGui::GetIO().DeltaTime,0.05f)));}
ImU32 uiColor(ImVec4 c,float alpha=1.0f){c.w*=alpha*ImGui::GetStyle().Alpha;return ImGui::ColorConvertFloat4ToU32(c);}
ImVec2 uiAdd(ImVec2 a,float x,float y){return ImVec2(a.x+x,a.y+y);}
void uiText(ImDrawList* d,ImVec2 center,const char* text,ImU32 color,float size=0,ImFont* font=nullptr){
    if(!font)font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;
    const ImVec2 t=font->CalcTextSizeA(size,FLT_MAX,0,text);
    d->AddText(font,size,ImVec2(center.x-t.x*0.5f,center.y-t.y*0.5f),color,text);
}
void uiTextLeft(ImDrawList* d,ImVec2 p,const char* text,ImU32 color,float size=0,ImFont* font=nullptr){
    if(!font)font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;d->AddText(font,size,p,color,text);
}
void uiIcon(ImDrawList* d,int icon,ImVec2 p,float size,ImU32 color){
    const auto& data=lemming::icons::kIcons[std::clamp(icon,0,8)];
    for(int j=0;j<data.count;++j){const auto& line=lemming::icons::kSegments[data.first+j];
        const ImVec2 a(p.x+line.x1*size/24,p.y+line.y1*size/24),b(p.x+line.x2*size/24,p.y+line.y2*size/24);
        d->AddLine(a,b,color,1.65f*size/24);d->AddCircleFilled(a,0.825f*size/24,color,6);
    }
}
void uiPanel(ImDrawList* d,ImVec2 a,ImVec2 b,float rounding=14){
    const float s = g_uiScale;
    const float r = rounding * s;

    // Item 5: Multi-layer soft shadows
    d->AddRectFilled(ImVec2(a.x - 4.0f * s, a.y - 4.0f * s), ImVec2(b.x + 4.0f * s, b.y + 16.0f * s), uiColor(ImVec4(0.0f, 0.0f, 0.0f, 0.35f)), r + 4.0f * s);
    d->AddRectFilled(a, ImVec2(b.x, b.y + 6.0f * s), uiColor(ImVec4(0.0f, 0.0f, 0.0f, 0.25f)), r);

    // Panel background
    d->AddRectFilled(a, b, uiColor(ImVec4(0.047f, 0.055f, 0.075f, 0.985f)), r);

    // Item 4: Gradient top edge cut
    const float midX = (a.x + b.x) * 0.5f;
    if (midX > a.x + r && b.x - r > midX) {
        const ImU32 colTrans = uiColor(g_appearance.accent, 0.0f);
        const ImU32 colAccent = uiColor(g_appearance.accent, 0.85f);
        d->AddRectFilledMultiColor(ImVec2(a.x + r, a.y), ImVec2(midX, a.y + 1.5f * s), colTrans, colAccent, colAccent, colTrans);
        d->AddRectFilledMultiColor(ImVec2(midX, a.y), ImVec2(b.x - r, a.y + 1.5f * s), colAccent, colTrans, colTrans, colAccent);
    }

    // Border
    d->AddRect(a, b, uiColor(ImVec4(0.19f, 0.22f, 0.29f, 0.65f)), r, 0, 1.0f);
}
void uiCard(ImDrawList* d,ImVec2 a,ImVec2 b){
    d->AddRectFilled(a,b,uiColor(ImVec4(.066f,.078f,.104f,1)),12*g_uiScale);
    d->AddRect(a,b,uiColor(ImVec4(.18f,.21f,.28f,.58f)),12*g_uiScale);
}
bool uiRailButton(const char* label,int icon,bool selected,float width){
    const float h=68*g_uiScale;const ImVec2 a=ImGui::GetCursorScreenPos();
    const bool pressed=ImGui::InvisibleButton(label,ImVec2(width,h));auto* d=ImGui::GetWindowDrawList();
    uiIcon(d,icon,uiAdd(a,(width-27*g_uiScale)*0.5f,9*g_uiScale),26*g_uiScale,uiColor(selected?g_appearance.accent:ImVec4(0.48f,0.54f,0.66f,1)));
    uiText(d,uiAdd(a,width*0.5f,52*g_uiScale),label,uiColor(selected?ImVec4(0.93f,0.95f,1,1):ImVec4(0.54f,0.60f,0.71f,1)),18*g_uiScale,g_uiSmallFont);
    return pressed;
}
bool uiToggle(const char* label,bool& value){
    ImGui::PushID(label);const ImVec2 a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=48*g_uiScale;
    const bool hit=ImGui::InvisibleButton("##switch",ImVec2(w,h));if(hit)value=!value;
    ImGuiStorage* store=ImGui::GetStateStorage();auto id=ImGui::GetItemID();
    float t=uiEase(store->GetFloat(id,value?1:0),value?1:0,20);store->SetFloat(id,t);
    auto* d=ImGui::GetWindowDrawList();const float sh=26*g_uiScale,sw=48*g_uiScale;
    const ImVec2 p=uiAdd(a,w-sw,0.5f*(h-sh));ImVec4 off(0.15f,0.18f,0.24f,1),on=g_appearance.accent;
    ImVec4 track(off.x+(on.x-off.x)*t,off.y+(on.y-off.y)*t,off.z+(on.z-off.z)*t,1);
    d->AddRectFilled(p,uiAdd(p,sw,sh),uiColor(track),sh*0.5f);
    d->AddCircleFilled(uiAdd(p,sh*0.5f+(sw-sh)*t,sh*0.5f),9.4f*g_uiScale,uiColor(ImVec4(0.97f,0.98f,1,1)),20);
    uiText(d,uiAdd(a,(w-sw-14*g_uiScale)*0.5f,h*0.5f),label,uiColor(ImVec4(0.85f,0.89f,0.96f,1)),19*g_uiScale);
    d->AddLine(uiAdd(a,0,h),uiAdd(a,w,h),uiColor(ImVec4(0.18f,0.20f,0.27f,0.40f)),1);
    ImGui::PopID();return hit;
}
void uiColorRow(const char* label,ImVec4& c){
    ImGui::PushID(label);const ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x;
    uiText(ImGui::GetWindowDrawList(),uiAdd(p,(w-42*g_uiScale)*0.5f,19*g_uiScale),label,uiColor(ImVec4(0.76f,0.81f,0.90f,1)),18*g_uiScale,g_uiSmallFont);
    ImGui::SetCursorScreenPos(uiAdd(p,w-34*g_uiScale,2*g_uiScale));
    if(ImGui::ColorButton("##color",c,ImGuiColorEditFlags_NoTooltip,ImVec2(32*g_uiScale,32*g_uiScale)))ImGui::OpenPopup("##picker");
    if(ImGui::BeginPopup("##picker")){
        ImGui::SetNextItemWidth(250*g_uiScale);
        ImGui::ColorPicker4("##rgba",&c.x,ImGuiColorEditFlags_AlphaBar|ImGuiColorEditFlags_DisplayHex|ImGuiColorEditFlags_NoSidePreview);
        ImGui::EndPopup();
    }
    ImGui::SetCursorScreenPos(uiAdd(p,0,44*g_uiScale));ImGui::PopID();
}
bool uiChoiceButton(const char* id,const char* label,bool selected,ImVec2 size){
    ImGui::PushID(id);const ImVec2 a=ImGui::GetCursorScreenPos();const bool pressed=ImGui::InvisibleButton("##choice",size);
    auto* d=ImGui::GetWindowDrawList();const ImVec4 bg=selected?ImVec4(.12f,.16f,.26f,1):ImVec4(.075f,.09f,.122f,1);
    d->AddRectFilled(a,uiAdd(a,size.x,size.y),uiColor(bg),8*g_uiScale);
    d->AddRect(a,uiAdd(a,size.x,size.y),uiColor(selected?g_appearance.accent:ImVec4(.18f,.21f,.28f,.75f),selected?.75f:1),8*g_uiScale);
    uiText(d,uiAdd(a,size.x*.5f,size.y*.5f),label,uiColor(selected?ImVec4(.95f,.97f,1,1):ImVec4(.68f,.73f,.82f,1)),17*g_uiScale,g_uiSmallFont);
    ImGui::PopID();return pressed;
}
void uiSlider(const char* id,const char* label,float& value,float lo,float hi,const char* format){
    ImGui::PushID(id);const ImVec2 a=ImGui::GetCursorScreenPos();
    uiTextLeft(ImGui::GetWindowDrawList(),a,label,uiColor(ImVec4(.68f,.73f,.82f,1)),16*g_uiScale,g_uiSmallFont);
    ImGui::SetCursorScreenPos(uiAdd(a,0,23*g_uiScale));ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::SliderFloat("##value",&value,lo,hi,format);
    ImGui::SetCursorScreenPos(uiAdd(a,0,64*g_uiScale));ImGui::PopID();
}
bool uiActionButton(const char* id,const char* title,const char* subtitle,int icon){
    ImGui::PushID(id);const ImVec2 a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=98*g_uiScale;
    const bool pressed=ImGui::InvisibleButton("##action",ImVec2(w,h));auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(a,uiAdd(a,w,h),uiColor(ImVec4(.075f,.09f,.122f,1)),12*g_uiScale);
    d->AddRect(a,uiAdd(a,w,h),uiColor(g_appearance.accent,.52f),12*g_uiScale);
    uiIcon(d,icon,uiAdd(a,24*g_uiScale,34*g_uiScale),30*g_uiScale,uiColor(g_appearance.accent));
    uiTextLeft(d,uiAdd(a,72*g_uiScale,23*g_uiScale),title,uiColor(ImVec4(.94f,.96f,1,1)),21*g_uiScale,g_uiTitleFont);
    uiTextLeft(d,uiAdd(a,72*g_uiScale,57*g_uiScale),subtitle,uiColor(ImVec4(.49f,.55f,.66f,1)),16*g_uiScale,g_uiSmallFont);
    uiIcon(d,6,uiAdd(a,w-43*g_uiScale,37*g_uiScale),24*g_uiScale,uiColor(ImVec4(.55f,.61f,.72f,1)));
    ImGui::PopID();return pressed;
}
void openUrl(const char* url) {
    if (!url || !url[0]) return;
    LOGI("openUrl requested: %s", url);

    bool opened = false;
    JNIEnv* env = getJniEnv();
    if (env) {
        jobject activity = getActivity(env);
        if (activity) {
            jclass uriClass = env->FindClass("android/net/Uri");
            if (uriClass && !clearJniException(env, "finding Uri class")) {
                jmethodID parseMethod = env->GetStaticMethodID(
                    uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
                if (parseMethod && !clearJniException(env, "finding Uri.parse")) {
                    jstring urlStr = env->NewStringUTF(url);
                    jobject uriObj = env->CallStaticObjectMethod(uriClass, parseMethod, urlStr);
                    if (urlStr) env->DeleteLocalRef(urlStr);
                    if (uriObj && !clearJniException(env, "calling Uri.parse")) {
                        jclass intentClass = env->FindClass("android/content/Intent");
                        if (intentClass && !clearJniException(env, "finding Intent class")) {
                            jfieldID actionViewField = env->GetStaticFieldID(
                                intentClass, "ACTION_VIEW", "Ljava/lang/String;");
                            if (actionViewField && !clearJniException(env, "finding Intent.ACTION_VIEW")) {
                                jobject actionView = env->GetStaticObjectField(intentClass, actionViewField);
                                jmethodID intentInit = env->GetMethodID(
                                    intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
                                if (intentInit && !clearJniException(env, "finding Intent constructor")) {
                                    jobject intent = env->NewObject(intentClass, intentInit, actionView, uriObj);
                                    if (intent && !clearJniException(env, "instantiating Intent")) {
                                        jmethodID addFlags = env->GetMethodID(
                                            intentClass, "addFlags", "(I)Landroid/content/Intent;");
                                        if (addFlags && !clearJniException(env, "finding addFlags")) {
                                            env->CallObjectMethod(intent, addFlags, static_cast<jint>(0x10000000)); // FLAG_ACTIVITY_NEW_TASK
                                            clearJniException(env, "calling addFlags");
                                        }
                                        jclass actClass = env->GetObjectClass(activity);
                                        jmethodID startActivity = actClass ? env->GetMethodID(
                                            actClass, "startActivity", "(Landroid/content/Intent;)V") : nullptr;
                                        if (startActivity && !clearJniException(env, "finding startActivity")) {
                                            env->CallVoidMethod(activity, startActivity, intent);
                                            if (!clearJniException(env, "calling startActivity")) {
                                                opened = true;
                                                LOGI("openUrl started activity successfully");
                                            }
                                        }
                                        if (actClass) env->DeleteLocalRef(actClass);
                                        env->DeleteLocalRef(intent);
                                    }
                                }
                                if (actionView) env->DeleteLocalRef(actionView);
                            }
                            env->DeleteLocalRef(intentClass);
                        }
                        env->DeleteLocalRef(uriObj);
                    }
                }
                env->DeleteLocalRef(uriClass);
            }
        }
    }

    if (!opened) {
        LOGI("openUrl fallback: copying to clipboard");
        ImGui::SetClipboardText(url);
        if (env) {
            jobject activity = getActivity(env);
            if (activity) {
                jclass toastClass = env->FindClass("android/widget/Toast");
                if (toastClass && !clearJniException(env, "finding Toast class")) {
                    jmethodID makeText = env->GetStaticMethodID(
                        toastClass, "makeText", "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
                    if (makeText && !clearJniException(env, "finding Toast.makeText")) {
                        jstring text = env->NewStringUTF("Link copied");
                        jobject toast = env->CallStaticObjectMethod(toastClass, makeText, activity, text, 0);
                        if (text) env->DeleteLocalRef(text);
                        if (toast && !clearJniException(env, "calling Toast.makeText")) {
                            jmethodID show = env->GetMethodID(toastClass, "show", "()V");
                            if (show && !clearJniException(env, "finding Toast.show")) {
                                env->CallVoidMethod(toast, show);
                                clearJniException(env, "calling Toast.show");
                            }
                            env->DeleteLocalRef(toast);
                        }
                    }
                    env->DeleteLocalRef(toastClass);
                }
            }
        }
    }
}
bool uiLinkRow(const char* id, const char* label, const char* url, const ImVec2& size) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##link", size);
    const bool hovered = ImGui::IsItemHovered();

    if (pressed) {
        openUrl(url);
    }

    auto* d = ImGui::GetWindowDrawList();
    const float s = g_uiScale;
    const ImVec4 bgCol = hovered ? ImVec4(0.12f, 0.16f, 0.24f, 0.75f) : ImVec4(0.065f, 0.08f, 0.11f, 0.70f);
    d->AddRectFilled(p, uiAdd(p, size.x, size.y), uiColor(bgCol), 8.0f * s);
    d->AddRect(p, uiAdd(p, size.x, size.y), uiColor(hovered ? g_appearance.accent : ImVec4(0.18f, 0.21f, 0.28f, 0.50f)), 8.0f * s);

    const ImVec4 textCol = hovered ? g_appearance.accent : ImVec4(0.85f, 0.89f, 0.96f, 1.0f);
    uiTextLeft(d, uiAdd(p, 16.0f * s, (size.y - 16.0f * s) * 0.5f), label, uiColor(textCol), 16.0f * s, g_uiSmallFont);

    if (hovered) {
        ImFont* font = g_uiSmallFont ? g_uiSmallFont : ImGui::GetFont();
        const float textW = font->CalcTextSizeA(16.0f * s, FLT_MAX, 0, label).x;
        const float underlineY = p.y + (size.y + 16.0f * s) * 0.5f + 1.0f * s;
        d->AddLine(ImVec2(p.x + 16.0f * s, underlineY), ImVec2(p.x + 16.0f * s + textW, underlineY), uiColor(g_appearance.accent), 1.2f * s);
    }

    uiIcon(d, 6, uiAdd(p, size.x - 30.0f * s, (size.y - 18.0f * s) * 0.5f), 18.0f * s, uiColor(hovered ? g_appearance.accent : ImVec4(0.50f, 0.56f, 0.67f, 1.0f)));

    ImGui::PopID();
    return pressed;
}
void uiEspText(ImDrawList* d,ImVec2 center,const char* text,ImU32 color,float size){
    uiText(d,uiAdd(center,0,1),text,IM_COL32(0,0,0,235),size,g_uiSmallFont);uiText(d,center,text,color,size,g_uiSmallFont);
}
void drawStyledEsp(ImDrawList* d,const EspScreenBox& b){
    const ImVec2 a(b.left,b.top),z(b.right,b.bottom);float h=z.y-a.y,w=z.x-a.x;if(h<8||w<2)return;
    const float center=(a.x+z.x)*0.5f,thick=g_appearance.lineWidth;
    const float rounding=g_appearance.rounded?g_appearance.cornerRadius*g_uiScale:0.0f;const auto boxColor=uiColor(g_appearance.boxColor);
    if(g_appearance.box){
        if(g_appearance.fill)d->AddRectFilled(a,z,uiColor(g_appearance.boxColor,std::clamp(g_appearance.fillIntensity,0.0f,0.35f)),rounding);
        if(g_appearance.boxStyle==0){
            d->AddRect(a,z,IM_COL32(0,0,0,170),rounding,0,thick+2);d->AddRect(a,z,boxColor,rounding,0,thick);
        }else{
            float l=std::min(w*0.27f,h*0.15f);const ImVec2 pts[4]={a,ImVec2(z.x,a.y),ImVec2(a.x,z.y),z};
            for(int i=0;i<4;++i){float dx=i%2?-l:l,dy=i>=2?-l:l;
                d->AddLine(pts[i],uiAdd(pts[i],dx,0),IM_COL32(0,0,0,190),thick+2);d->AddLine(pts[i],uiAdd(pts[i],0,dy),IM_COL32(0,0,0,190),thick+2);
                d->AddLine(pts[i],uiAdd(pts[i],dx,0),boxColor,thick);d->AddLine(pts[i],uiAdd(pts[i],0,dy),boxColor,thick);
            }
        }
    }
    char text[48];
    if(g_appearance.health&&b.statsValid){
        const float ratio=b.maxHp?std::clamp(float(b.hp)/b.maxHp,0.0f,1.0f):0;const ImVec2 u(a.x-9,a.y),v(a.x-5,z.y);
        d->AddRectFilled(uiAdd(u,-1,-1),uiAdd(v,1,1),IM_COL32(5,8,14,220),3);d->AddRectFilled(u,v,IM_COL32(35,41,53,220),2);
        ImVec4 c=g_appearance.healthColor,low(1.0f,0.34f,0.40f,1);c=ImVec4(low.x+(c.x-low.x)*ratio,low.y+(c.y-low.y)*ratio,low.z+(c.z-low.z)*ratio,1);
        d->AddRectFilled(ImVec2(u.x,z.y-h*ratio),v,uiColor(c),2);
        if(b.hp<b.maxHp){std::snprintf(text,sizeof(text),"%u",unsigned(b.hp));uiEspText(d,ImVec2(a.x-20,z.y-h*ratio),text,uiColor(c),14*g_uiScale);}
    }
    float labelY=z.y+13*g_uiScale;
    if(g_appearance.armor&&b.statsValid&&b.maxArmor>0){
        float ratio=std::clamp(float(b.armor)/b.maxArmor,0.0f,1.0f);d->AddRectFilled(ImVec2(a.x-1,z.y+5),ImVec2(z.x+1,z.y+10),IM_COL32(5,8,14,210),3);
        if(ratio>0)d->AddRectFilled(ImVec2(a.x,z.y+6),ImVec2(a.x+w*ratio,z.y+9),uiColor(g_appearance.armorColor),2);labelY+=9*g_uiScale;
    }
    if(b.weaponValid&&g_appearance.weapon){std::snprintf(text,sizeof(text),"WEAPON %u",unsigned(b.weaponId));uiEspText(d,ImVec2(center,labelY),text,uiColor(g_appearance.textColor),16*g_uiScale);labelY+=19*g_uiScale;}
    if(b.weaponValid&&g_appearance.ammo){std::snprintf(text,sizeof(text),"%u / %u",unsigned(b.magazineAmmo),unsigned(b.reserveAmmo));uiEspText(d,ImVec2(center,labelY),text,uiColor(g_appearance.textColor,.73f),14*g_uiScale);}
}
void renderEspBoxes(){
    if(!g_espBoxesEnabled.load(std::memory_order_relaxed))return;std::lock_guard<std::mutex> lock(g_espMutex);auto* d=ImGui::GetBackgroundDrawList();
    for(const auto& b:g_espScreenBoxes)drawStyledEsp(d,b);
}
void renderWatermark(){
    auto& io=ImGui::GetIO();const float s=g_uiScale,w=std::round(180*s),h=std::round(56*s);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x-w)*0.5f,22*s),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin("##watermark",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground);
    const bool clicked=ImGui::InvisibleButton("##toggle_menu",ImVec2(w,h));const ImVec2 p=ImGui::GetItemRectMin();auto* d=ImGui::GetWindowDrawList();uiPanel(d,p,uiAdd(p,w,h),15);
    g_wmLeft.store(p.x, std::memory_order_relaxed);
    g_wmTop.store(p.y, std::memory_order_relaxed);
    g_wmRight.store(p.x + w, std::memory_order_relaxed);
    g_wmBottom.store(p.y + h, std::memory_order_relaxed);

    // Item 2: Pulsing status dot
    const ImVec2 dotPos(p.x + 18.0f * s, p.y + h * 0.5f);
    const float pulse = 0.65f + 0.35f * std::sin(float(ImGui::GetTime()) * 3.5f);
    d->AddCircleFilled(dotPos, 6.0f * s, uiColor(ImVec4(0.20f, 0.90f, 0.55f, 0.25f * pulse)));
    d->AddCircleFilled(dotPos, 3.5f * s, uiColor(ImVec4(0.20f, 0.90f, 0.55f, pulse)));

    const float tw=ImGui::GetFont()->CalcTextSizeA(21*s,FLT_MAX,0,"Nyx").x,total=tw+34*s,x=(w-total)*0.5f + 20.0f * s;
    uiIcon(d,8,uiAdd(p,x,16*s),24*s,uiColor(g_appearance.accent));uiText(d,uiAdd(p,x+34*s+tw*.5f,h*.5f),"Nyx",uiColor(ImVec4(.94f,.96f,1,1)),21*s);
    if(clicked){const bool open=!g_menuOpen.load(std::memory_order_relaxed);g_menuOpen.store(open,std::memory_order_relaxed);LOGI("menu watermark toggle: open=%d",open);}
    ImGui::End();ImGui::PopStyleVar();
}
void renderEspEditor(){
    if(!g_espEditorOpen)return;auto& io=ImGui::GetIO();const float s=g_uiScale,w=std::round(980*s),h=std::round(650*s);
    ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x-w)*.5f,(io.DisplaySize.y-h)*.5f),ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(14*s,14*s));ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,14*s);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,7*s));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.040f,.047f,.064f,.995f));ImGui::PushStyleColor(ImGuiCol_TitleBg,ImVec4(.060f,.072f,.100f,1));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,ImVec4(.060f,.072f,.100f,1));ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed,ImVec4(.060f,.072f,.100f,1));
    auto flags=ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse;
    if(ImGui::Begin("ESP Editor##esp_editor",&g_espEditorOpen,flags)){
        const ImVec2 base=ImGui::GetCursorScreenPos();const ImVec2 avail=ImGui::GetContentRegionAvail();auto* d=ImGui::GetWindowDrawList();
        const float gap=14*s,previewW=294*s,colW=(avail.x-previewW-gap*2)*.5f,cardH=avail.y;
        const ImVec2 geomA=base,geomB=uiAdd(geomA,colW,cardH);const ImVec2 colorA=uiAdd(base,colW+gap,0),colorB=uiAdd(colorA,colW,cardH);
        const ImVec2 prevA=uiAdd(base,(colW+gap)*2,0),prevB=uiAdd(prevA,previewW,cardH);uiCard(d,geomA,geomB);uiCard(d,colorA,colorB);uiCard(d,prevA,prevB);
        uiText(d,ImVec2((geomA.x+geomB.x)*.5f,geomA.y+24*s),"GEOMETRY",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        uiText(d,ImVec2((colorA.x+colorB.x)*.5f,colorA.y+24*s),"ELEMENTS & COLORS",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        uiText(d,ImVec2((prevA.x+prevB.x)*.5f,prevA.y+24*s),"ESP PREVIEW",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        ImGui::SetCursorScreenPos(uiAdd(geomA,14*s,48*s));ImGui::BeginChild("##esp_geometry",ImVec2(colW-28*s,cardH-62*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        const float choiceW=(ImGui::GetContentRegionAvail().x-8*s)*.5f;if(uiChoiceButton("full","Full",g_appearance.boxStyle==0,ImVec2(choiceW,44*s)))g_appearance.boxStyle=0;
        ImGui::SameLine(0,8*s);if(uiChoiceButton("corners","Corners",g_appearance.boxStyle==1,ImVec2(choiceW,44*s)))g_appearance.boxStyle=1;
        uiToggle("Box",g_appearance.box);uiToggle("Rounded",g_appearance.rounded);uiToggle("Soft fill",g_appearance.fill);
        uiSlider("thickness","Thickness",g_appearance.lineWidth,1.0f,4.0f,"%.1f px");
        uiSlider("fill_intensity","Fill intensity",g_appearance.fillIntensity,0.02f,0.30f,"%.2f");
        uiSlider("corner_radius","Corner radius",g_appearance.cornerRadius,0.0f,18.0f,"%.0f px");ImGui::EndChild();
        ImGui::SetCursorScreenPos(uiAdd(colorA,14*s,48*s));ImGui::BeginChild("##esp_colors",ImVec2(colW-28*s,cardH-62*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        uiToggle("Health",g_appearance.health);uiToggle("Armor",g_appearance.armor);uiToggle("Weapon",g_appearance.weapon);uiToggle("Ammo",g_appearance.ammo);
        uiColorRow("Box color",g_appearance.boxColor);uiColorRow("Health color",g_appearance.healthColor);uiColorRow("Armor color",g_appearance.armorColor);
        uiColorRow("Text color",g_appearance.textColor);uiColorRow("Accent",g_appearance.accent);ImGui::EndChild();
        EspScreenBox demo{};demo.left=prevA.x+91*s;demo.right=prevB.x-67*s;demo.top=prevA.y+88*s;demo.bottom=prevB.y-126*s;
        demo.statsValid=demo.weaponValid=demo.enemy=true;demo.hp=76;demo.maxHp=100;demo.armor=64;demo.maxArmor=100;demo.weaponId=12;demo.magazineAmmo=30;demo.reserveAmmo=90;
        const float cx=(demo.left+demo.right)*.5f,cy=demo.top+35*s;const ImU32 mannequin=uiColor(ImVec4(.24f,.29f,.38f,.50f));
        d->AddCircleFilled(ImVec2(cx,cy),13*s,mannequin,24);
        d->AddRectFilled(ImVec2(cx-25*s,cy+18*s),ImVec2(cx+25*s,cy+112*s),mannequin,12*s);
        d->AddRectFilled(ImVec2(cx-39*s,cy+30*s),ImVec2(cx-27*s,cy+105*s),mannequin,6*s);d->AddRectFilled(ImVec2(cx+27*s,cy+30*s),ImVec2(cx+39*s,cy+105*s),mannequin,6*s);
        d->AddRectFilled(ImVec2(cx-22*s,cy+105*s),ImVec2(cx-4*s,cy+190*s),mannequin,8*s);d->AddRectFilled(ImVec2(cx+4*s,cy+105*s),ImVec2(cx+22*s,cy+190*s),mannequin,8*s);drawStyledEsp(d,demo);
        uiText(d,ImVec2((prevA.x+prevB.x)*.5f,prevB.y-28*s),"Drag by the title bar",uiColor(ImVec4(.45f,.51f,.62f,1)),14*s,g_uiSmallFont);
    }
    ImGui::End();ImGui::PopStyleColor(4);ImGui::PopStyleVar(3);
}
#include "ui/pages/aim.hpp"

void uiSectionChange(int section,int sub){if(g_section!=section||g_subsection!=sub){g_section=section;g_subsection=sub;g_pageAnimation=0;}}
void renderWatermarkOverlay(){
    static float g_wm_alpha = 1.0f;
    const float targetAlpha = g_watermark_enabled ? 1.0f : 0.0f;
    g_wm_alpha = uiEase(g_wm_alpha, targetAlpha, 15.0f);
    if(g_wm_alpha <= 0.005f) return;

    auto& io = ImGui::GetIO();
    const float s = g_uiScale;

    static float g_lastFpsTime = 0.0f;
    static float g_displayFps = 60.0f;
    const float curTime = float(ImGui::GetTime());
    if (curTime - g_lastFpsTime >= 0.5f) {
        g_displayFps = io.Framerate;
        g_lastFpsTime = curTime;
    }

    char fpsBuf[32];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", g_displayFps);

    ImFont* font = g_uiSmallFont ? g_uiSmallFont : ImGui::GetFont();
    const float iconW = 12.0f * s;
    const float nyxW = font->CalcTextSizeA(15.0f * s, FLT_MAX, 0, "Nyx").x;
    const float sep1W = font->CalcTextSizeA(14.0f * s, FLT_MAX, 0, "|").x;
    const float verW = font->CalcTextSizeA(13.0f * s, FLT_MAX, 0, "v1.0.0").x;
    const float sep2W = font->CalcTextSizeA(14.0f * s, FLT_MAX, 0, "|").x;
    const float fpsW = font->CalcTextSizeA(13.0f * s, FLT_MAX, 0, fpsBuf).x;

    const float pad = 12.0f * s;
    const float gap = 8.0f * s;
    const float totalW = pad * 2.0f + iconW + gap + nyxW + gap + sep1W + gap + verW + (g_show_fps ? (gap + sep2W + gap + fpsW) : 0.0f);
    const float cardH = 36.0f * s;
    const float cardX = io.DisplaySize.x - 12.0f * s - totalW;
    const float cardY = 12.0f * s;

    auto* dl = ImGui::GetForegroundDrawList();
    const ImVec2 p(cardX, cardY);
    const ImVec2 pMax(cardX + totalW, cardY + cardH);

    dl->AddRectFilled(p, pMax, uiColor(ImVec4(0.04f, 0.05f, 0.07f, g_watermark_opacity * g_wm_alpha)), 9.0f * s);
    const ImVec4 borderCol(g_watermark_accent[0], g_watermark_accent[1], g_watermark_accent[2], 0.40f * g_wm_alpha);
    dl->AddRect(p, pMax, uiColor(borderCol), 9.0f * s, 0, 1.0f);

    const float centerY = cardY + cardH * 0.5f;
    float curX = cardX + pad;

    const ImVec4 iconCol(g_watermark_accent[0], g_watermark_accent[1], g_watermark_accent[2], g_wm_alpha);
    uiIcon(dl, 8, ImVec2(curX, centerY - 6.0f * s), 12.0f * s, uiColor(iconCol));
    curX += iconW + gap;

    const float nyxH = font->CalcTextSizeA(15.0f * s, FLT_MAX, 0, "Nyx").y;
    uiTextLeft(dl, ImVec2(curX, centerY - nyxH * 0.5f), "Nyx", uiColor(ImVec4(1.0f, 1.0f, 1.0f, g_wm_alpha)), 15.0f * s, font);
    curX += nyxW + gap;

    const float sepH = font->CalcTextSizeA(14.0f * s, FLT_MAX, 0, "|").y;
    uiTextLeft(dl, ImVec2(curX, centerY - sepH * 0.5f), "|", uiColor(ImVec4(0.40f, 0.45f, 0.55f, g_wm_alpha)), 14.0f * s, font);
    curX += sep1W + gap;

    const float verH = font->CalcTextSizeA(13.0f * s, FLT_MAX, 0, "v1.0.0").y;
    uiTextLeft(dl, ImVec2(curX, centerY - verH * 0.5f), "v1.0.0", uiColor(ImVec4(0.60f, 0.65f, 0.75f, g_wm_alpha)), 13.0f * s, font);
    curX += verW + gap;

    if (g_show_fps) {
        uiTextLeft(dl, ImVec2(curX, centerY - sepH * 0.5f), "|", uiColor(ImVec4(0.40f, 0.45f, 0.55f, g_wm_alpha)), 14.0f * s, font);
        curX += sep2W + gap;

        ImVec4 fpsCol;
        if (g_displayFps >= 50.0f) {
            fpsCol = ImVec4(0.30f, 0.85f, 0.45f, g_wm_alpha);
        } else if (g_displayFps >= 30.0f) {
            fpsCol = ImVec4(0.95f, 0.75f, 0.25f, g_wm_alpha);
        } else {
            fpsCol = ImVec4(0.95f, 0.30f, 0.35f, g_wm_alpha);
        }
        const float fpsH = font->CalcTextSizeA(13.0f * s, FLT_MAX, 0, fpsBuf).y;
        uiTextLeft(dl, ImVec2(curX, centerY - fpsH * 0.5f), fpsBuf, uiColor(fpsCol), 13.0f * s, font);
    }
}

void renderMinimalMenu(){
    auto& io=ImGui::GetIO();g_uiScale=std::min({io.DisplaySize.y/1080.0f,io.DisplaySize.x/1440.0f,io.DisplaySize.y/760.0f,1.25f});
    g_uiScale=std::max(0.25f,g_uiScale);io.FontGlobalScale=g_uiScale;renderEspBoxes();renderAngleFov();renderWatermark();lemming::aim::g_menuOpen.store(g_menuOpen.load(),std::memory_order_release);
    const bool open=g_menuOpen.load(std::memory_order_relaxed);g_menuAnimation=uiEase(g_menuAnimation,open?1:0);

    // Item 1: Backdrop Dimmer
    if(g_menuAnimation > 0.01f){
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0.0f,0.0f),io.DisplaySize,uiColor(ImVec4(0.0f,0.0f,0.0f,0.35f*g_menuAnimation)));
    }

    if(g_menuAnimation >= 0.005f || open){
        if(!is_logged_in){
            const float s=g_uiScale,lw=std::round(460.0f*s),lh=std::round(450.0f*s);
            const float lx=std::floor((io.DisplaySize.x-lw)*0.5f),ly=std::floor((io.DisplaySize.y-lh)*0.5f)+(1.0f-g_menuAnimation)*14.0f*s;
            if(open){
                g_menuLeft.store(lx,std::memory_order_relaxed);
                g_menuTop.store(ly,std::memory_order_relaxed);
                g_menuRight.store(lx+lw,std::memory_order_relaxed);
                g_menuBottom.store(ly+lh,std::memory_order_relaxed);
            }else{
                g_menuLeft.store(0.0f,std::memory_order_relaxed);
                g_menuTop.store(0.0f,std::memory_order_relaxed);
                g_menuRight.store(0.0f,std::memory_order_relaxed);
                g_menuBottom.store(0.0f,std::memory_order_relaxed);
            }
            auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground;
            if(!open){
                flags|=ImGuiWindowFlags_NoInputs;
                JNIEnv* env = getJniEnv();
                imeHide(env);
            }
            ImGui::SetNextWindowPos(ImVec2(lx,ly),ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(lw,lh),ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(24.0f*s,20.0f*s));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0.0f,10.0f*s));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,8.0f*s);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(14.0f*s,10.0f*s));
            ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.075f,0.09f,0.125f,1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0.11f,0.14f,0.20f,1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,ImVec4(0.14f,0.18f,0.26f,1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.94f,0.96f,1.0f,1.0f));

            ImGui::Begin("##login_screen",nullptr,flags);
            auto* d=ImGui::GetWindowDrawList();
            uiPanel(d,ImVec2(lx,ly),ImVec2(lx+lw,ly+lh),17);

            uiIcon(d,8,ImVec2(lx+lw*0.5f-20.0f*s,ly+28.0f*s),40.0f*s,uiColor(g_appearance.accent));
            uiText(d,ImVec2(lx+lw*0.5f,ly+84.0f*s),"Nyx Authorization",uiColor(ImVec4(0.94f,0.96f,1.0f,1.0f)),25.0f*s,g_uiTitleFont);
            uiText(d,ImVec2(lx+lw*0.5f,ly+108.0f*s),"Enter credentials to access workspace",uiColor(ImVec4(0.48f,0.54f,0.66f,1.0f)),15.0f*s,g_uiSmallFont);
            d->AddLine(ImVec2(lx+24.0f*s,ly+128.0f*s),ImVec2(lx+lw-24.0f*s,ly+128.0f*s),uiColor(ImVec4(0.18f,0.21f,0.29f,0.65f)),1.0f);

            uiTextLeft(d,ImVec2(lx+32.0f*s,ly+144.0f*s),"Operator ID",uiColor(ImVec4(0.68f,0.74f,0.85f,1.0f)),15.0f*s,g_uiSmallFont);
            ImGui::SetCursorPos(ImVec2(32.0f*s,164.0f*s));
            ImGui::SetNextItemWidth(lw-64.0f*s);
            ImGui::InputText("##operator_id",login_input,sizeof(login_input));
            // Item 6: Input Glow Aura
            if (ImGui::IsItemActive()) {
                const ImVec2 rMin = ImGui::GetItemRectMin();
                const ImVec2 rMax = ImGui::GetItemRectMax();
                d->AddRect(ImVec2(rMin.x - 2.0f * s, rMin.y - 2.0f * s), ImVec2(rMax.x + 2.0f * s, rMax.y + 2.0f * s), uiColor(g_appearance.accent, 0.50f), 10.0f * s, 0, 2.0f * s);
                d->AddRect(ImVec2(rMin.x - 4.0f * s, rMin.y - 4.0f * s), ImVec2(rMax.x + 4.0f * s, rMax.y + 4.0f * s), uiColor(g_appearance.accent, 0.20f), 12.0f * s, 0, 1.5f * s);
            }
            if (ImGui::IsItemActivated()) {
                JNIEnv* env = getJniEnv();
                imeShow(env, 1, login_input);
            }

            uiTextLeft(d,ImVec2(lx+32.0f*s,ly+220.0f*s),"Access Key",uiColor(ImVec4(0.68f,0.74f,0.85f,1.0f)),15.0f*s,g_uiSmallFont);
            ImGui::SetCursorPos(ImVec2(32.0f*s,240.0f*s));
            ImGui::SetNextItemWidth(lw-64.0f*s);
            const bool enterPressed=ImGui::InputText("##access_key",password_input,sizeof(password_input),ImGuiInputTextFlags_Password|ImGuiInputTextFlags_EnterReturnsTrue);
            // Item 6: Input Glow Aura
            if (ImGui::IsItemActive()) {
                const ImVec2 rMin = ImGui::GetItemRectMin();
                const ImVec2 rMax = ImGui::GetItemRectMax();
                d->AddRect(ImVec2(rMin.x - 2.0f * s, rMin.y - 2.0f * s), ImVec2(rMax.x + 2.0f * s, rMax.y + 2.0f * s), uiColor(g_appearance.accent, 0.50f), 10.0f * s, 0, 2.0f * s);
                d->AddRect(ImVec2(rMin.x - 4.0f * s, rMin.y - 4.0f * s), ImVec2(rMax.x + 4.0f * s, rMax.y + 4.0f * s), uiColor(g_appearance.accent, 0.20f), 12.0f * s, 0, 1.5f * s);
            }
            if (ImGui::IsItemActivated()) {
                JNIEnv* env = getJniEnv();
                imeShow(env, 2, password_input);
            }

            if(show_login_error){
                uiText(d,ImVec2(lx+lw*0.5f,ly+292.0f*s),"Access Denied",uiColor(ImVec4(0.96f,0.29f,0.36f,1.0f)),16.0f*s,g_uiSmallFont);
            }

            ImGui::SetCursorPos(ImVec2(32.0f*s,314.0f*s));
            const ImVec2 btnSize(lw-64.0f*s,48.0f*s);
            const ImVec2 btnPos=ImGui::GetCursorScreenPos();
            const bool verifyClicked=ImGui::InvisibleButton("##verify_btn",btnSize);
            const bool hovered=ImGui::IsItemHovered();
            const bool active=ImGui::IsItemActive();

            ImVec4 btnColor=g_appearance.accent;
            if(active)btnColor=ImVec4(btnColor.x*0.80f,btnColor.y*0.80f,btnColor.z*0.80f,1.0f);
            else if(hovered)btnColor=ImVec4(std::min(1.0f,btnColor.x*1.15f),std::min(1.0f,btnColor.y*1.15f),std::min(1.0f,btnColor.z*1.15f),1.0f);

            d->AddRectFilled(btnPos,uiAdd(btnPos,btnSize.x,btnSize.y),uiColor(btnColor),9.0f*s);
            uiText(d,uiAdd(btnPos,btnSize.x*0.5f,btnSize.y*0.5f),"Verify",uiColor(ImVec4(0.04f,0.06f,0.10f,1.0f)),20.0f*s,g_uiTitleFont);

            if(verifyClicked||enterPressed||g_imeEnterPressed.exchange(false, std::memory_order_relaxed)){
                if(std::strcmp(login_input,"admin")==0&&std::strcmp(password_input,"admin")==0){
                    is_logged_in=true;
                    show_login_error=false;
                    g_pageAnimation=0.0f;
                    JNIEnv* env = getJniEnv();
                    imeHide(env);
                }else{
                    show_login_error=true;
                }
            }

            ImGui::SetCursorPos(ImVec2(32.0f*s,374.0f*s));
            const ImVec2 guestPos=ImGui::GetCursorScreenPos();
            const bool guestClicked=ImGui::InvisibleButton("##guest_btn",btnSize);
            const bool guestHovered=ImGui::IsItemHovered();
            const bool guestActive=ImGui::IsItemActive();

            ImVec4 guestBg(0.11f,0.14f,0.20f,1.0f);
            if(guestActive)guestBg=ImVec4(0.08f,0.10f,0.15f,1.0f);
            else if(guestHovered)guestBg=ImVec4(0.15f,0.19f,0.27f,1.0f);

            d->AddRectFilled(guestPos,uiAdd(guestPos,btnSize.x,btnSize.y),uiColor(guestBg),9.0f*s);
            d->AddRect(guestPos,uiAdd(guestPos,btnSize.x,btnSize.y),uiColor(ImVec4(0.20f,0.24f,0.32f,0.70f)),9.0f*s,0,1.0f);
            uiText(d,uiAdd(guestPos,btnSize.x*0.5f,btnSize.y*0.5f),"Continue as Guest",uiColor(ImVec4(0.85f,0.89f,0.96f,1.0f)),18.0f*s,g_uiTitleFont);

            if(guestClicked){
                is_logged_in=true;
                show_login_error=false;
                login_input[0]='\0';
                password_input[0]='\0';
                g_pageAnimation=0.0f;
                JNIEnv* env=getJniEnv();
                imeHide(env);
            }

            ImGui::End();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(5);
        } else {
            g_pageAnimation=uiEase(g_pageAnimation,1,15);const float s=g_uiScale;
            const float baseW = std::round(820.0f * s * g_window_scale);
            const float baseH = std::round(650.0f * s * g_window_scale);

            if ((g_window_pos.x == 0.0f && g_window_pos.y == 0.0f) || g_reset_window_pos) {
                g_window_pos = ImVec2(std::floor((io.DisplaySize.x - baseW) * 0.5f), std::floor((io.DisplaySize.y - baseH) * 0.5f));
                g_window_size = ImVec2(baseW, baseH);
            }

            float x = g_window_pos.x;
            float y = g_window_pos.y;
            float w = g_window_size.x;
            float h = g_window_size.y;

            const char* sectionNames[]={"General","Aim","Visuals","Misc","Config","Settings"};
            const int sectionIcons[]={8,0,1,2,5,7};
            const int sectionCount=6;
            const float primaryH=(24+sectionCount*74)*s;

            const char* labels[6][6]={
                {"Overview",nullptr,nullptr,nullptr,nullptr,nullptr},
                {"Angles","Silent","Targeting","Multipoints","Automation","Prediction"},
                {"Players","Chams","World","Appearance",nullptr,nullptr},
                {"General","Movement","Other",nullptr,nullptr,nullptr},
                {"Profiles","Import","Export",nullptr,nullptr,nullptr},
                {"Theme","About",nullptr,nullptr,nullptr,nullptr}
            };
            const int icons[6][6]={
                {8,0,0,0,0,0},
                {0,0,3,2,0,0},
                {3,4,4,5,0,0},
                {2,2,7,0,0,0},
                {5,6,6,0,0,0},
                {5,8,0,0,0,0}
            };

            auto mainFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;
            if(!open) mainFlags |= ImGuiWindowFlags_NoInputs;
            if(g_window_locked) {
                mainFlags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
            } else {
                mainFlags |= ImGuiWindowFlags_NoTitleBar;
            }

            if (g_window_locked || g_reset_window_pos) {
                ImGui::SetNextWindowPos(ImVec2(x,y), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(w,h), ImGuiCond_Always);
                g_reset_window_pos = false;
            } else {
                ImGui::SetNextWindowPos(ImVec2(x,y), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(w,h), ImGuiCond_FirstUseEver);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12*s,12*s));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,6*s));

            ImGui::Begin("##main",nullptr,mainFlags);
            auto* d=ImGui::GetWindowDrawList();
            if (!g_window_locked) {
                g_window_pos = ImGui::GetWindowPos();
                g_window_size = ImGui::GetWindowSize();
                x = g_window_pos.x;
                y = g_window_pos.y;
                w = g_window_size.x;
                h = g_window_size.y;
            }

            int n=0;while(n<6&&labels[g_section][n])++n;
            const float secondaryH=(24+n*74)*s;

            if(open){
                g_menuLeft.store(x - 180.0f * s, std::memory_order_relaxed);
                g_menuTop.store(y, std::memory_order_relaxed);
                g_menuRight.store(x + w + 195.0f * s, std::memory_order_relaxed);
                g_menuBottom.store(y + std::max({h, primaryH, secondaryH}), std::memory_order_relaxed);
            } else {
                g_menuLeft.store(0.0f, std::memory_order_relaxed);
                g_menuTop.store(0.0f, std::memory_order_relaxed);
                g_menuRight.store(0.0f, std::memory_order_relaxed);
                g_menuBottom.store(0.0f, std::memory_order_relaxed);
            }

            auto railFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
            if(!open) railFlags |= ImGuiWindowFlags_NoInputs;

            // Render ##sections synchronized with ##main
            ImGui::SetNextWindowPos(ImVec2(x-176*s,y),ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(156*s,primaryH),ImGuiCond_Always);
            ImGui::Begin("##sections",nullptr,railFlags);
            auto* secD = ImGui::GetWindowDrawList();
            uiPanel(secD,ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),156*s,primaryH));

            static float g_navPillY = -1.0f;
            const float targetPillY = ImGui::GetWindowPos().y + 12.0f * s + g_section * 74.0f * s;
            if(g_navPillY < 0.0f) g_navPillY = targetPillY;
            g_navPillY = uiEase(g_navPillY, targetPillY, 22.0f);
            const float pillX = ImGui::GetWindowPos().x + 12.0f * s;
            const float pillW = 132.0f * s;
            const float pillH = 68.0f * s;
            secD->AddRectFilled(ImVec2(pillX, g_navPillY), ImVec2(pillX + pillW, g_navPillY + pillH), uiColor(ImVec4(0.12f, 0.16f, 0.24f, 0.90f)), 11.0f * s);
            secD->AddRectFilled(ImVec2(pillX, g_navPillY + 12.0f * s), ImVec2(pillX + 3.5f * s, g_navPillY + pillH - 12.0f * s), uiColor(g_appearance.accent), 1.75f * s);

            for(int i=0;i<sectionCount;++i)if(uiRailButton(sectionNames[i],sectionIcons[i],g_section==i,132*s))uiSectionChange(i,0);
            ImGui::End();

            // Render ##subsections synchronized with ##main
            ImGui::SetNextWindowPos(ImVec2(x+w+20*s,y),ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(172*s,secondaryH),ImGuiCond_Always);
            ImGui::Begin("##subsections",nullptr,railFlags);
            auto* subD = ImGui::GetWindowDrawList();
            uiPanel(subD,ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),172*s,secondaryH));

            static float g_subNavPillY = -1.0f;
            const float targetSubPillY = ImGui::GetWindowPos().y + 12.0f * s + g_subsection * 74.0f * s;
            if(g_subNavPillY < 0.0f) g_subNavPillY = targetSubPillY;
            g_subNavPillY = uiEase(g_subNavPillY, targetSubPillY, 22.0f);
            const float subPillX = ImGui::GetWindowPos().x + 12.0f * s;
            const float subPillW = 148.0f * s;
            const float subPillH = 68.0f * s;
            subD->AddRectFilled(ImVec2(subPillX, g_subNavPillY), ImVec2(subPillX + subPillW, g_subNavPillY + subPillH), uiColor(ImVec4(0.12f, 0.16f, 0.24f, 0.90f)), 11.0f * s);
            subD->AddRectFilled(ImVec2(subPillX, g_subNavPillY + 12.0f * s), ImVec2(subPillX + 3.5f * s, g_subNavPillY + subPillH - 12.0f * s), uiColor(g_appearance.accent), 1.75f * s);

            for(int i=0;i<n;++i)if(uiRailButton(labels[g_section][i],icons[g_section][i],g_subsection==i,148*s))uiSectionChange(g_section,i);
            ImGui::End();

            uiPanel(d,ImVec2(x,y),ImVec2(x+w,y+h),17);
            char title[72];std::snprintf(title,sizeof(title),"%s / %s",sectionNames[g_section],labels[g_section][g_subsection]);
            uiText(d,ImVec2(x+w*.5f,y+40*s),title,uiColor(ImVec4(.94f,.96f,1,1)),29*s,g_uiTitleFont);
            const char* subTitleText = "Nyx";
            if (g_section == 0) subTitleText = "Overview & Links";
            else if (g_section == 2 && g_subsection == 0) subTitleText = "Player overlays and detailed styling.";
            uiText(d,ImVec2(x+w*.5f,y+72*s),subTitleText,uiColor(ImVec4(.49f,.55f,.66f,1)),17*s,g_uiSmallFont);
            d->AddLine(ImVec2(x+28*s,y+100*s),ImVec2(x+w-28*s,y+100*s),uiColor(ImVec4(.18f,.21f,.29f,.65f)),1);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation*(0.18f+.82f*g_pageAnimation));const float slide=(1-g_pageAnimation)*10*s;
            if(g_section==0){
                // General
                const float cx = x + w * 0.5f;

                // 1. Large Nyx icon (40*s) with double glow circles
                const ImVec2 iconCenter(cx, y + 155.0f * s + slide);
                d->AddCircleFilled(iconCenter, 32.0f * s, uiColor(g_appearance.accent, 0.10f), 32);
                d->AddCircleFilled(iconCenter, 24.0f * s, uiColor(g_appearance.accent, 0.18f), 32);
                uiIcon(d, 8, ImVec2(iconCenter.x - 20.0f * s, iconCenter.y - 20.0f * s), 40.0f * s, uiColor(g_appearance.accent));

                // 2. "Nyx" - g_uiTitleFont, 40*s, white, with light accent glow
                const ImVec2 titleCenter(cx, y + 205.0f * s + slide);
                uiText(d, ImVec2(titleCenter.x - 1.0f * s, titleCenter.y), "Nyx", uiColor(g_appearance.accent, 0.35f), 40.0f * s, g_uiTitleFont);
                uiText(d, ImVec2(titleCenter.x + 1.0f * s, titleCenter.y), "Nyx", uiColor(g_appearance.accent, 0.35f), 40.0f * s, g_uiTitleFont);
                uiText(d, ImVec2(titleCenter.x, titleCenter.y - 1.0f * s), "Nyx", uiColor(g_appearance.accent, 0.35f), 40.0f * s, g_uiTitleFont);
                uiText(d, ImVec2(titleCenter.x, titleCenter.y + 1.0f * s), "Nyx", uiColor(g_appearance.accent, 0.35f), 40.0f * s, g_uiTitleFont);
                uiText(d, titleCenter, "Nyx", uiColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), 40.0f * s, g_uiTitleFont);

                // 3. "v1.0.0" - g_uiSmallFont, 16*s, muted grey
                uiText(d, ImVec2(cx, y + 242.0f * s + slide), "v1.0.0", uiColor(ImVec4(0.60f, 0.65f, 0.75f, 1.0f)), 16.0f * s, g_uiSmallFont);

                // 4. "ARM64 / GLES3 overlay" - 15*s, even quieter
                uiText(d, ImVec2(cx, y + 268.0f * s + slide), "ARM64 / GLES3 overlay", uiColor(ImVec4(0.42f, 0.48f, 0.58f, 1.0f)), 15.0f * s, g_uiSmallFont);

                // 5. Separator 60% window width, accent alpha 0.4
                const float sepW = w * 0.60f;
                const float sepY = y + 300.0f * s + slide;
                d->AddLine(ImVec2(cx - sepW * 0.5f, sepY), ImVec2(cx + sepW * 0.5f, sepY), uiColor(g_appearance.accent, 0.40f), 1.0f);

                // 6. Section "Links"
                uiText(d, ImVec2(cx, y + 326.0f * s + slide), "Links", uiColor(ImVec4(0.68f, 0.74f, 0.85f, 1.0f)), 15.0f * s, g_uiSmallFont);

                const float linkW = std::min(w * 0.60f, 420.0f * s);
                const float linkX = cx - linkW * 0.5f;

                ImGui::SetCursorScreenPos(ImVec2(linkX, y + 350.0f * s + slide));
                uiLinkRow("tg_link", "Telegram: @eco1kd", "https://t.me/eco1kd", ImVec2(linkW, 44.0f * s));

                ImGui::SetCursorScreenPos(ImVec2(linkX, y + 404.0f * s + slide));
                uiLinkRow("gh_link", "GitHub: https://github.com/eco1kd", "https://github.com/eco1kd", ImVec2(linkW, 44.0f * s));

                // 7. Bottom center: "© 2026 Nyx"
                uiText(d, ImVec2(cx, y + h - 28.0f * s), "© 2026 Nyx", uiColor(ImVec4(0.40f, 0.45f, 0.55f, 0.80f)), 13.0f * s, g_uiSmallFont);
            }else if(g_section==1){
                renderAimControls(x,y,slide);
            }else if(g_section==2&&g_subsection==0){
                // Visuals -> Players
                const ImVec2 coreA(x+30*s,y+124*s),coreB(x+395*s,y+474*s),detailA(x+425*s,y+124*s),detailB(x+790*s,y+474*s);uiCard(d,coreA,coreB);uiCard(d,detailA,detailB);
                uiText(d,ImVec2((coreA.x+coreB.x)*.5f,coreA.y+25*s),"ESP CORE",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
                uiText(d,ImVec2((detailA.x+detailB.x)*.5f,detailA.y+25*s),"OVERLAYS",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
                ImGui::SetCursorPos(ImVec2(47*s,165*s+slide));ImGui::BeginChild("##core_controls",ImVec2(331*s,286*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                bool enabled=g_espBoxesEnabled.load();if(uiToggle("Enemy ESP",enabled)){g_espBoxesEnabled.store(enabled);LOGI("ESP appearance master: enabled=%d",enabled);}
                uiToggle("Box",g_appearance.box);uiToggle("Soft fill",g_appearance.fill);uiToggle("Rounded",g_appearance.rounded);ImGui::EndChild();
                ImGui::SetCursorPos(ImVec2(442*s,165*s+slide));ImGui::BeginChild("##detail_controls",ImVec2(331*s,286*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiToggle("Health",g_appearance.health);uiToggle("Armor",g_appearance.armor);uiToggle("Weapon",g_appearance.weapon);uiToggle("Ammo",g_appearance.ammo);ImGui::EndChild();
                ImGui::SetCursorPos(ImVec2(30*s,500*s+slide));if(uiActionButton("esp_editor","Open ESP Editor","Style, thickness, fill, rounding, colors and preview",5))g_espEditorOpen=true;
            }else if(g_section==2&&g_subsection==1){
                // Visuals -> Chams
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##visuals_chams",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiToggle("Enemy Chams",g_chams_enemy);
                uiToggle("Team Chams",g_chams_team);
                uiToggle("Wireframe",g_chams_wireframe);
                ImGui::EndChild();
            }else if(g_section==2&&g_subsection==2){
                // Visuals -> World
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##visuals_world",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiToggle("Night Mode",g_world_nightmode);
                uiSlider("world_bright","Brightness",g_world_brightness,0.1f,2.0f,"%.2f");
                uiToggle("Remove Fog",g_world_remove_fog);
                ImGui::EndChild();
            }else if(g_section==2&&g_subsection==3){
                // Visuals -> Appearance: Watermark Settings
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##appearance_action",ImVec2(628*s,360*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                if(uiActionButton("appearance_editor","Open ESP Editor","Move the panel and tune every visual detail",5))g_espEditorOpen=true;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+14*s);
                uiToggle("Show Watermark",g_watermark_enabled);
                uiSlider("wm_opacity","Watermark Opacity",g_watermark_opacity,0.3f,1.0f,"%.2f");
                ImVec4 wmColor(g_watermark_accent[0],g_watermark_accent[1],g_watermark_accent[2],g_watermark_accent[3]);
                uiColorRow("Watermark Accent",wmColor);
                g_watermark_accent[0]=wmColor.x;g_watermark_accent[1]=wmColor.y;g_watermark_accent[2]=wmColor.z;g_watermark_accent[3]=wmColor.w;
                ImGui::EndChild();
                uiText(d,ImVec2(x+w*.5f,y+530*s),"The editor contains the live ESP preview and all appearance controls.",uiColor(ImVec4(.50f,.56f,.67f,1)),17*s,g_uiSmallFont);
            }else if(g_section==3&&g_subsection==0){
                // Misc -> General
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##misc_general",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiToggle("No Flash",g_misc_no_flash);
                uiToggle("No Smoke",g_misc_no_smoke);
                uiToggle("Third Person",g_misc_third_person);
                uiSlider("tp_dist","Distance",g_misc_third_person_dist,50.0f,400.0f,"%.0f");
                uiSlider("fov","FOV",g_misc_fov,60.0f,120.0f,"%.0f deg");
                uiToggle("Hit Sound",g_misc_hit_sound);
                ImGui::EndChild();
            }else if(g_section==3&&g_subsection==1){
                // Misc -> Movement
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##misc_movement",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiToggle("Bunnyhop",g_misc_bhop);
                uiToggle("Auto Strafe",g_misc_auto_strafe);
                uiToggle("Fast Crouch",g_misc_fast_crouch);
                ImGui::EndChild();
            }else if(g_section==3&&g_subsection==2){
                // Misc -> Other
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##misc_other",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiToggle("Fast Switch",g_misc_fast_switch);
                uiToggle("Quick Reload",g_misc_quick_reload);
                uiToggle("Anti-Screenshot",g_misc_anti_screenshot);
                ImGui::EndChild();
            }else if(g_section==4&&g_subsection==0){
                // Config -> Profiles
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##config_profiles",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiTextLeft(ImGui::GetWindowDrawList(),ImGui::GetCursorScreenPos(),"Preset Name",uiColor(ImVec4(0.68f,0.74f,0.85f,1.0f)),15*s,g_uiSmallFont);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+20*s);
                ImGui::SetNextItemWidth(628*s);
                ImGui::InputText("##cfg_name",config_name,sizeof(config_name));
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+10*s);

                const float btnW=(628*s-16*s)/3.0f;
                if(uiChoiceButton("cfg_save","Save",false,ImVec2(btnW,40*s))){}
                ImGui::SameLine(0,8*s);
                if(uiChoiceButton("cfg_load","Load",false,ImVec2(btnW,40*s))){}
                ImGui::SameLine(0,8*s);
                if(uiChoiceButton("cfg_del","Delete",false,ImVec2(btnW,40*s))){}
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+18*s);

                uiTextLeft(ImGui::GetWindowDrawList(),ImGui::GetCursorScreenPos(),"Available Presets",uiColor(ImVec4(0.68f,0.74f,0.85f,1.0f)),15*s,g_uiSmallFont);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+22*s);

                const char* presetList[5]={"default.cfg","legit_competitive.cfg","rage_hvhh.cfg","sniper_pro.cfg","movement_trick.cfg"};
                for(int i=0;i<5;++i){
                    char idBuf[32];std::snprintf(idBuf,sizeof(idBuf),"preset_%d",i);
                    if(uiChoiceButton(idBuf,presetList[i],g_selected_preset==i,ImVec2(628*s,36*s))){
                        g_selected_preset=i;
                        std::snprintf(config_name,sizeof(config_name),"%s",presetList[i]);
                    }
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY()+4*s);
                }

                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+10*s);
                uiTextLeft(ImGui::GetWindowDrawList(),ImGui::GetCursorScreenPos(),"Configs stored in app data dir",uiColor(ImVec4(0.48f,0.54f,0.66f,1.0f)),14*s,g_uiSmallFont);
                ImGui::EndChild();
            }else if(g_section==4&&g_subsection==1){
                // Config -> Import
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##config_import",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                static char import_code[128]="";
                uiTextLeft(ImGui::GetWindowDrawList(),ImGui::GetCursorScreenPos(),"Import Code / Preset String",uiColor(ImVec4(0.68f,0.74f,0.85f,1.0f)),15*s,g_uiSmallFont);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+20*s);
                ImGui::SetNextItemWidth(628*s);
                ImGui::InputText("##import_code",import_code,sizeof(import_code));
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+12*s);
                uiChoiceButton("import_btn","Import from Clipboard",false,ImVec2(628*s,42*s));
                ImGui::EndChild();
            }else if(g_section==4&&g_subsection==2){
                // Config -> Export
                ImGui::SetCursorPos(ImVec2(96*s,130*s+slide));ImGui::BeginChild("##config_export",ImVec2(628*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
                uiChoiceButton("export_btn","Export to Clipboard",false,ImVec2(628*s,42*s));
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+16*s);
                uiTextLeft(ImGui::GetWindowDrawList(),ImGui::GetCursorScreenPos(),"Configuration string ready to share with team.",uiColor(ImVec4(0.48f,0.54f,0.66f,1.0f)),15*s,g_uiSmallFont);
                ImGui::EndChild();
            }else if(g_section==5&&g_subsection==0){
                // Settings -> Theme
                ImGui::SetCursorPos(ImVec2(116*s,130*s+slide));ImGui::BeginChild("##theme",ImVec2(588*s,470*s),ImGuiChildFlags_None,ImGuiWindowFlags_None);
                uiColorRow("Accent",g_appearance.accent);
                uiColorRow("Panel text",g_appearance.textColor);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+8*s);
                float prevWinScale = g_window_scale;
                uiSlider("win_scale","Window Scale",g_window_scale,0.6f,1.5f,"%.2f");
                if (std::abs(g_window_scale - prevWinScale) > 0.001f) {
                    g_window_size = ImVec2(std::round(820.0f * s * g_window_scale), std::round(650.0f * s * g_window_scale));
                    g_reset_window_pos = true;
                }
                uiToggle("Lock Window", g_window_locked);
                uiToggle("Show FPS", g_show_fps);
                if (uiChoiceButton("reset_pos", "Reset Position", false, ImVec2(ImGui::GetContentRegionAvail().x, 38.0f * s))) {
                    g_window_scale = 1.0f;
                    g_reset_window_pos = true;
                }
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+10*s);
                uiSlider("ui_scale","UI Scale",g_user_ui_scale,0.8f,1.5f,"%.2f");
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+8*s);
                uiTextLeft(ImGui::GetWindowDrawList(),ImGui::GetCursorScreenPos(),"Language",uiColor(ImVec4(0.68f,0.73f,0.82f,1.0f)),16*s,g_uiSmallFont);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()+24*s);
                const char* languages[]={"English","Русский","中文","Deutsch"};
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::Combo("##language",&g_selected_language,languages,IM_ARRAYSIZE(languages));
                ImGui::EndChild();
            }else if(g_section==5&&g_subsection==1){
                // Settings -> About
                uiIcon(d,8,ImVec2(x+w*.5f-25*s,y+224*s+slide),50*s,uiColor(g_appearance.accent));
                uiText(d,ImVec2(x+w*.5f,y+320*s+slide),"Nyx",uiColor(ImVec4(.94f,.96f,1,1)),34*s,g_uiTitleFont);
                uiText(d,ImVec2(x+w*.5f,y+365*s+slide),"v1.0.0",uiColor(ImVec4(.60f,.65f,.75f,1)),18*s,g_uiSmallFont);
            }
            ImGui::PopStyleVar();ImGui::End();ImGui::PopStyleVar(3);if(open&&g_espEditorOpen)renderEspEditor();
        }
    } else {
        g_menuLeft.store(0.0f, std::memory_order_relaxed);
        g_menuTop.store(0.0f, std::memory_order_relaxed);
        g_menuRight.store(0.0f, std::memory_order_relaxed);
        g_menuBottom.store(0.0f, std::memory_order_relaxed);
    }

    // Item 7: Watermark Overlay
    renderWatermarkOverlay();
}
