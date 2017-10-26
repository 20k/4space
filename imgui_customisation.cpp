#include "imgui_customisation.hpp"

bool ImGui::BeginOverride(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    return ImGui::Begin(name, p_open, flags);
}

bool ImGui::BeginOverride(const char* name, bool* p_open, const ImVec2& size_on_first_use, float bg_alpha, ImGuiWindowFlags flags)
{
    return ImGui::Begin(name, p_open, size_on_first_use, bg_alpha, flags);
}

#if 0

#include "imgui_customisation.hpp"
#include "ui_util.hpp"
#define IMGUI_DEFINE_PLACEMENT_NEW
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <SFML/Graphics.hpp>

static ImGuiIniData* FindWindowSettings(const char* name)
{
    ImGuiContext& g = *GImGui;
    ImGuiID id = ImHash(name, 0);
    for (int i = 0; i != g.Settings.Size; i++)
    {
        ImGuiIniData* ini = &g.Settings[i];
        if (ini->Id == id)
            return ini;
    }
    return NULL;
}

static ImGuiIniData* AddWindowSettings(const char* name)
{
    GImGui->Settings.resize(GImGui->Settings.Size + 1);
    ImGuiIniData* ini = &GImGui->Settings.back();
    ini->Name = ImStrdup(name);
    ini->Id = ImHash(name, 0);
    ini->Collapsed = false;
    ini->Pos = ImVec2(FLT_MAX,FLT_MAX);
    ini->Size = ImVec2(0,0);
    return ini;
}

static ImGuiWindow* CreateNewWindow(const char* name, ImVec2 size, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;

    // Create window the first time
    ImGuiWindow* window = (ImGuiWindow*)ImGui::MemAlloc(sizeof(ImGuiWindow));
    IM_PLACEMENT_NEW(window) ImGuiWindow(name);
    window->Flags = flags;

    if (flags & ImGuiWindowFlags_NoSavedSettings)
    {
        // User can disable loading and saving of settings. Tooltip and child windows also don't store settings.
        window->Size = window->SizeFull = size;
    }
    else
    {
        // Retrieve settings from .ini file
        // Use SetWindowPos() or SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
        window->PosFloat = ImVec2(60, 60);
        window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);

        ImGuiIniData* settings = FindWindowSettings(name);
        if (!settings)
        {
            settings = AddWindowSettings(name);
        }
        else
        {
            window->SetWindowPosAllowFlags &= ~ImGuiSetCond_FirstUseEver;
            window->SetWindowSizeAllowFlags &= ~ImGuiSetCond_FirstUseEver;
            window->SetWindowCollapsedAllowFlags &= ~ImGuiSetCond_FirstUseEver;
        }

        if (settings->Pos.x != FLT_MAX)
        {
            window->PosFloat = settings->Pos;
            window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);
            window->Collapsed = settings->Collapsed;
        }

        if (ImLengthSqr(settings->Size) > 0.00001f && !(flags & ImGuiWindowFlags_NoResize))
            size = settings->Size;
        window->Size = window->SizeFull = size;
    }

    if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
    {
        window->AutoFitFramesX = window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
    else
    {
        if (window->Size.x <= 0.0f)
            window->AutoFitFramesX = 2;
        if (window->Size.y <= 0.0f)
            window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
    }

    if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
        g.Windows.insert(g.Windows.begin(), window); // Quite slow but rare and only once
    else
        g.Windows.push_back(window);
    return window;
}

bool ImGui::BeginOverride(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
    return ImGui::BeginOverride(name, p_open, ImVec2(0.f, 0.f), -1.0f, flags);
}

static void SetCurrentWindow(ImGuiWindow* window)
{
    ImGuiContext& g = *GImGui;
    g.CurrentWindow = window;
    if (window)
        g.FontSize = window->CalcFontSize();
}

// Save and compare stack sizes on Begin()/End() to detect usage errors
static void CheckStacksSize(ImGuiWindow* window, bool write)
{
    // NOT checking: DC.ItemWidth, DC.AllowKeyboardFocus, DC.ButtonRepeat, DC.TextWrapPos (per window) to allow user to conveniently push once and not pop (they are cleared on Begin)
    ImGuiContext& g = *GImGui;
    int* p_backup = &window->DC.StackSizesBackup[0];
    { int current = window->IDStack.Size;       if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "PushID/PopID or TreeNode/TreePop Mismatch!");   p_backup++; }    // Too few or too many PopID()/TreePop()
    { int current = window->DC.GroupStack.Size; if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "BeginGroup/EndGroup Mismatch!");                p_backup++; }    // Too few or too many EndGroup()
    { int current = g.CurrentPopupStack.Size;   if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "BeginMenu/EndMenu or BeginPopup/EndPopup Mismatch"); p_backup++;}// Too few or too many EndMenu()/EndPopup()
    { int current = g.ColorModifiers.Size;      if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "PushStyleColor/PopStyleColor Mismatch!");       p_backup++; }    // Too few or too many PopStyleColor()
    { int current = g.StyleModifiers.Size;      if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "PushStyleVar/PopStyleVar Mismatch!");           p_backup++; }    // Too few or too many PopStyleVar()
    { int current = g.FontStack.Size;           if (write) *p_backup = current; else IM_ASSERT(*p_backup == current && "PushFont/PopFont Mismatch!");                   p_backup++; }    // Too few or too many PopFont()
    IM_ASSERT(p_backup == window->DC.StackSizesBackup + IM_ARRAYSIZE(window->DC.StackSizesBackup));
}

static void SetWindowPos(ImGuiWindow* window, const ImVec2& pos, ImGuiSetCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowPosAllowFlags & cond) == 0)
        return;
    window->SetWindowPosAllowFlags &= ~(ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver | ImGuiSetCond_Appearing);
    window->SetWindowPosCenterWanted = false;

    // Set
    const ImVec2 old_pos = window->Pos;
    window->PosFloat = pos;
    window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);
    window->DC.CursorPos += (window->Pos - old_pos);    // As we happen to move the window while it is being appended to (which is a bad idea - will smear) let's at least offset the cursor
    window->DC.CursorMaxPos += (window->Pos - old_pos); // And more importantly we need to adjust this so size calculation doesn't get affected.
}

static void SetWindowSize(ImGuiWindow* window, const ImVec2& size, ImGuiSetCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowSizeAllowFlags & cond) == 0)
        return;
    window->SetWindowSizeAllowFlags &= ~(ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver | ImGuiSetCond_Appearing);

    // Set
    if (size.x > 0.0f)
    {
        window->AutoFitFramesX = 0;
        window->SizeFull.x = size.x;
    }
    else
    {
        window->AutoFitFramesX = 2;
        window->AutoFitOnlyGrows = false;
    }
    if (size.y > 0.0f)
    {
        window->AutoFitFramesY = 0;
        window->SizeFull.y = size.y;
    }
    else
    {
        window->AutoFitFramesY = 2;
        window->AutoFitOnlyGrows = false;
    }
}

static void SetWindowCollapsed(ImGuiWindow* window, bool collapsed, ImGuiSetCond cond)
{
    // Test condition (NB: bit 0 is always true) and clear flags for next time
    if (cond && (window->SetWindowCollapsedAllowFlags & cond) == 0)
        return;
    window->SetWindowCollapsedAllowFlags &= ~(ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver | ImGuiSetCond_Appearing);

    // Set
    window->Collapsed = collapsed;
}

static ImRect GetVisibleRect()
{
    ImGuiContext& g = *GImGui;
    if (g.IO.DisplayVisibleMin.x != g.IO.DisplayVisibleMax.x && g.IO.DisplayVisibleMin.y != g.IO.DisplayVisibleMax.y)
        return ImRect(g.IO.DisplayVisibleMin, g.IO.DisplayVisibleMax);
    return ImRect(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y);
}


static void MarkIniSettingsDirty()
{
    ImGuiContext& g = *GImGui;
    if (g.SettingsDirtyTimer <= 0.0f)
        g.SettingsDirtyTimer = g.IO.IniSavingRate;
}

static void ApplySizeFullWithConstraint(ImGuiWindow* window, ImVec2 new_size)
{
    ImGuiContext& g = *GImGui;
    if (g.SetNextWindowSizeConstraint)
    {
        // Using -1,-1 on either X/Y axis to preserve the current size.
        ImRect cr = g.SetNextWindowSizeConstraintRect;
        new_size.x = (cr.Min.x >= 0 && cr.Max.x >= 0) ? ImClamp(new_size.x, cr.Min.x, cr.Max.x) : window->SizeFull.x;
        new_size.y = (cr.Min.y >= 0 && cr.Max.y >= 0) ? ImClamp(new_size.y, cr.Min.y, cr.Max.y) : window->SizeFull.y;
        if (g.SetNextWindowSizeConstraintCallback)
        {
            ImGuiSizeConstraintCallbackData data;
            data.UserData = g.SetNextWindowSizeConstraintCallbackUserData;
            data.Pos = window->Pos;
            data.CurrentSize = window->SizeFull;
            data.DesiredSize = new_size;
            g.SetNextWindowSizeConstraintCallback(&data);
            new_size = data.DesiredSize;
        }
    }
    if (!(window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysAutoResize)))
        new_size = ImMax(new_size, g.Style.WindowMinSize);
    window->SizeFull = new_size;
}

static ImVec2 FindBestPopupWindowPos(const ImVec2& base_pos, const ImVec2& size, int* last_dir, const ImRect& r_inner)
{
    const ImGuiStyle& style = GImGui->Style;

    // Clamp into visible area while not overlapping the cursor. Safety padding is optional if our popup size won't fit without it.
    ImVec2 safe_padding = style.DisplaySafeAreaPadding;
    ImRect r_outer(GetVisibleRect());
    r_outer.Reduce(ImVec2((size.x - r_outer.GetWidth() > safe_padding.x*2) ? safe_padding.x : 0.0f, (size.y - r_outer.GetHeight() > safe_padding.y*2) ? safe_padding.y : 0.0f));
    ImVec2 base_pos_clamped = ImClamp(base_pos, r_outer.Min, r_outer.Max - size);

    for (int n = (*last_dir != -1) ? -1 : 0; n < 4; n++)   // Last, Right, down, up, left. (Favor last used direction).
    {
        const int dir = (n == -1) ? *last_dir : n;
        ImRect rect(dir == 0 ? r_inner.Max.x : r_outer.Min.x, dir == 1 ? r_inner.Max.y : r_outer.Min.y, dir == 3 ? r_inner.Min.x : r_outer.Max.x, dir == 2 ? r_inner.Min.y : r_outer.Max.y);
        if (rect.GetWidth() < size.x || rect.GetHeight() < size.y)
            continue;
        *last_dir = dir;
        return ImVec2(dir == 0 ? r_inner.Max.x : dir == 3 ? r_inner.Min.x - size.x : base_pos_clamped.x, dir == 1 ? r_inner.Max.y : dir == 2 ? r_inner.Min.y - size.y : base_pos_clamped.y);
    }

    // Fallback, try to keep within display
    *last_dir = -1;
    ImVec2 pos = base_pos;
    pos.x = ImMax(ImMin(pos.x + size.x, r_outer.Max.x) - size.x, r_outer.Min.x);
    pos.y = ImMax(ImMin(pos.y + size.y, r_outer.Max.y) - size.y, r_outer.Min.y);
    return pos;
}

static ImGuiWindow* GetFrontMostModalRootWindow()
{
    ImGuiContext& g = *GImGui;
    for (int n = g.OpenPopupStack.Size-1; n >= 0; n--)
        if (ImGuiWindow* front_most_popup = g.OpenPopupStack.Data[n].Window)
            if (front_most_popup->Flags & ImGuiWindowFlags_Modal)
                return front_most_popup;
    return NULL;
}

static void Scrollbar(ImGuiWindow* window, bool horizontal)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(horizontal ? "#SCROLLX" : "#SCROLLY");

    // Render background
    bool other_scrollbar = (horizontal ? window->ScrollbarY : window->ScrollbarX);
    float other_scrollbar_size_w = other_scrollbar ? style.ScrollbarSize : 0.0f;
    const ImRect window_rect = window->Rect();
    const float border_size = window->BorderSize;
    ImRect bb = horizontal
        ? ImRect(window->Pos.x + border_size, window_rect.Max.y - style.ScrollbarSize, window_rect.Max.x - other_scrollbar_size_w - border_size, window_rect.Max.y - border_size)
        : ImRect(window_rect.Max.x - style.ScrollbarSize, window->Pos.y + border_size, window_rect.Max.x - border_size, window_rect.Max.y - other_scrollbar_size_w - border_size);
    if (!horizontal)
        bb.Min.y += window->TitleBarHeight() + ((window->Flags & ImGuiWindowFlags_MenuBar) ? window->MenuBarHeight() : 0.0f);
    if (bb.GetWidth() <= 0.0f || bb.GetHeight() <= 0.0f)
        return;

    float window_rounding = (window->Flags & ImGuiWindowFlags_ChildWindow) ? style.ChildWindowRounding : style.WindowRounding;
    int window_rounding_corners;
    if (horizontal)
        window_rounding_corners = ImGuiCorner_BottomLeft | (other_scrollbar ? 0 : ImGuiCorner_BottomRight);
    else
        window_rounding_corners = (((window->Flags & ImGuiWindowFlags_NoTitleBar) && !(window->Flags & ImGuiWindowFlags_MenuBar)) ? ImGuiCorner_TopRight : 0) | (other_scrollbar ? 0 : ImGuiCorner_BottomRight);
    window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_ScrollbarBg), window_rounding, window_rounding_corners);
    bb.Reduce(ImVec2(ImClamp((float)(int)((bb.Max.x - bb.Min.x - 2.0f) * 0.5f), 0.0f, 3.0f), ImClamp((float)(int)((bb.Max.y - bb.Min.y - 2.0f) * 0.5f), 0.0f, 3.0f)));

    // V denote the main axis of the scrollbar
    float scrollbar_size_v = horizontal ? bb.GetWidth() : bb.GetHeight();
    float scroll_v = horizontal ? window->Scroll.x : window->Scroll.y;
    float win_size_avail_v = (horizontal ? window->Size.x : window->Size.y) - other_scrollbar_size_w;
    float win_size_contents_v = horizontal ? window->SizeContents.x : window->SizeContents.y;

    // The grabable box size generally represent the amount visible (vs the total scrollable amount)
    // But we maintain a minimum size in pixel to allow for the user to still aim inside.
    const float grab_h_pixels = ImMin(ImMax(scrollbar_size_v * ImSaturate(win_size_avail_v / ImMax(win_size_contents_v, win_size_avail_v)), style.GrabMinSize), scrollbar_size_v);
    const float grab_h_norm = grab_h_pixels / scrollbar_size_v;

    // Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
    bool held = false;
    bool hovered = false;
    const bool previously_held = (g.ActiveId == id);
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    float scroll_max = ImMax(1.0f, win_size_contents_v - win_size_avail_v);
    float scroll_ratio = ImSaturate(scroll_v / scroll_max);
    float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;
    if (held && grab_h_norm < 1.0f)
    {
        float scrollbar_pos_v = horizontal ? bb.Min.x : bb.Min.y;
        float mouse_pos_v = horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
        float* click_delta_to_grab_center_v = horizontal ? &g.ScrollbarClickDeltaToGrabCenter.x : &g.ScrollbarClickDeltaToGrabCenter.y;

        // Click position in scrollbar normalized space (0.0f->1.0f)
        const float clicked_v_norm = ImSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);
        ImGui::SetHoveredID(id);

        bool seek_absolute = false;
        if (!previously_held)
        {
            // On initial click calculate the distance between mouse and the center of the grab
            if (clicked_v_norm >= grab_v_norm && clicked_v_norm <= grab_v_norm + grab_h_norm)
            {
                *click_delta_to_grab_center_v = clicked_v_norm - grab_v_norm - grab_h_norm*0.5f;
            }
            else
            {
                seek_absolute = true;
                *click_delta_to_grab_center_v = 0.0f;
            }
        }

        // Apply scroll
        // It is ok to modify Scroll here because we are being called in Begin() after the calculation of SizeContents and before setting up our starting position
        const float scroll_v_norm = ImSaturate((clicked_v_norm - *click_delta_to_grab_center_v - grab_h_norm*0.5f) / (1.0f - grab_h_norm));
        scroll_v = (float)(int)(0.5f + scroll_v_norm * scroll_max);//(win_size_contents_v - win_size_v));
        if (horizontal)
            window->Scroll.x = scroll_v;
        else
            window->Scroll.y = scroll_v;

        // Update values for rendering
        scroll_ratio = ImSaturate(scroll_v / scroll_max);
        grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;

        // Update distance to grab now that we have seeked and saturated
        if (seek_absolute)
            *click_delta_to_grab_center_v = clicked_v_norm - grab_v_norm - grab_h_norm*0.5f;
    }

    // Render
    const ImU32 grab_col = ImGui::GetColorU32(held ? ImGuiCol_ScrollbarGrabActive : hovered ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab);
    if (horizontal)
        window->DrawList->AddRectFilled(ImVec2(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y), ImVec2(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, bb.Max.y), grab_col, style.ScrollbarRounding);
    else
        window->DrawList->AddRectFilled(ImVec2(bb.Min.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm)), ImVec2(bb.Max.x, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels), grab_col, style.ScrollbarRounding);
}

// Upper-right button to close a window.
bool ImGui::CloseButtonOverride(ImGuiID id, const ImVec2& pos, float radius)
{
    ImGuiWindow* window = GetCurrentWindow();

    const ImRect bb(pos - ImVec2(radius,radius), pos + ImVec2(radius,radius));

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    // Render
    const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_CloseButtonActive : hovered ? ImGuiCol_CloseButtonHovered : ImGuiCol_CloseButton);
    const ImVec2 center = bb.GetCenter();
    //window->DrawList->AddCircleFilled(center, ImMax(2.0f, radius), col, 12);

    ImVec2 offset = ImVec2(ImMax(2.f, radius), ImMax(2.f, radius));

    window->DrawList->AddRectFilled(center - offset, center + offset, col, ImGui::GetStyle().WindowRounding);

    const float cross_extent = (radius * 0.7071f) - 1.0f;
    if (hovered)
    {
        window->DrawList->AddLine(center + ImVec2(+cross_extent,+cross_extent), center + ImVec2(-cross_extent,-cross_extent), GetColorU32(ImGuiCol_Text));
        window->DrawList->AddLine(center + ImVec2(+cross_extent,-cross_extent), center + ImVec2(-cross_extent,+cross_extent), GetColorU32(ImGuiCol_Text));
    }

    return pressed;
}

bool ImGui::BeginOverride(const char* name, bool* p_open, const ImVec2& size_on_first_use, float bg_alpha, ImGuiWindowFlags flags)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    IM_ASSERT(name != NULL);                        // Window name required
    IM_ASSERT(g.Initialized);                       // Forgot to call ImGui::NewFrame()
    IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

    if (flags & ImGuiWindowFlags_NoInputs)
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    // Find or create
    bool window_is_new = false;
    ImGuiWindow* window = FindWindowByName(name);
    if (!window)
    {
        window = CreateNewWindow(name, size_on_first_use, flags);
        window_is_new = true;
    }

    const int current_frame = ImGui::GetFrameCount();
    const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);
    if (first_begin_of_the_frame)
        window->Flags = (ImGuiWindowFlags)flags;
    else
        flags = window->Flags;

    // Add to stack
    ImGuiWindow* parent_window = !g.CurrentWindowStack.empty() ? g.CurrentWindowStack.back() : NULL;
    g.CurrentWindowStack.push_back(window);
    SetCurrentWindow(window);
    CheckStacksSize(window, true);
    IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));

    bool window_was_active = (window->LastFrameActive == current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
    if (flags & ImGuiWindowFlags_Popup)
    {
        ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.CurrentPopupStack.Size];
        window_was_active &= (window->PopupId == popup_ref.PopupId);
        window_was_active &= (window == popup_ref.Window);
        popup_ref.Window = window;
        g.CurrentPopupStack.push_back(popup_ref);
        window->PopupId = popup_ref.PopupId;
    }

    const bool window_appearing_after_being_hidden = (window->HiddenFrames == 1);

    // Process SetNextWindow***() calls
    bool window_pos_set_by_api = false, window_size_set_by_api = false;
    if (g.SetNextWindowPosCond)
    {
        const ImVec2 backup_cursor_pos = window->DC.CursorPos;                  // FIXME: not sure of the exact reason of this saving/restore anymore :( need to look into that.
        if (!window_was_active || window_appearing_after_being_hidden) window->SetWindowPosAllowFlags |= ImGuiSetCond_Appearing;
        window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.SetNextWindowPosCond) != 0;
        if (window_pos_set_by_api && ImLengthSqr(g.SetNextWindowPosVal - ImVec2(-FLT_MAX,-FLT_MAX)) < 0.001f)
        {
            window->SetWindowPosCenterWanted = true;                            // May be processed on the next frame if this is our first frame and we are measuring size
            window->SetWindowPosAllowFlags &= ~(ImGuiSetCond_Once | ImGuiSetCond_FirstUseEver | ImGuiSetCond_Appearing);
        }
        else
        {
            SetWindowPos(window, g.SetNextWindowPosVal, g.SetNextWindowPosCond);
        }
        window->DC.CursorPos = backup_cursor_pos;
        g.SetNextWindowPosCond = 0;
    }
    if (g.SetNextWindowSizeCond)
    {
        if (!window_was_active || window_appearing_after_being_hidden) window->SetWindowSizeAllowFlags |= ImGuiSetCond_Appearing;
        window_size_set_by_api = (window->SetWindowSizeAllowFlags & g.SetNextWindowSizeCond) != 0;
        SetWindowSize(window, g.SetNextWindowSizeVal, g.SetNextWindowSizeCond);
        g.SetNextWindowSizeCond = 0;
    }
    if (g.SetNextWindowContentSizeCond)
    {
        window->SizeContentsExplicit = g.SetNextWindowContentSizeVal;
        g.SetNextWindowContentSizeCond = 0;
    }
    else if (first_begin_of_the_frame)
    {
        window->SizeContentsExplicit = ImVec2(0.0f, 0.0f);
    }
    if (g.SetNextWindowCollapsedCond)
    {
        if (!window_was_active || window_appearing_after_being_hidden) window->SetWindowCollapsedAllowFlags |= ImGuiSetCond_Appearing;
        SetWindowCollapsed(window, g.SetNextWindowCollapsedVal, g.SetNextWindowCollapsedCond);
        g.SetNextWindowCollapsedCond = 0;
    }
    if (g.SetNextWindowFocus)
    {
        ImGui::SetWindowFocus();
        g.SetNextWindowFocus = false;
    }

    // Update known root window (if we are a child window, otherwise window == window->RootWindow)
    int root_idx, root_non_popup_idx;
    for (root_idx = g.CurrentWindowStack.Size - 1; root_idx > 0; root_idx--)
        if (!(g.CurrentWindowStack[root_idx]->Flags & ImGuiWindowFlags_ChildWindow))
            break;
    for (root_non_popup_idx = root_idx; root_non_popup_idx > 0; root_non_popup_idx--)
        if (!(g.CurrentWindowStack[root_non_popup_idx]->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)))
            break;
    window->ParentWindow = parent_window;
    window->RootWindow = g.CurrentWindowStack[root_idx];
    window->RootNonPopupWindow = g.CurrentWindowStack[root_non_popup_idx];      // This is merely for displaying the TitleBgActive color.

    // When reusing window again multiple times a frame, just append content (don't need to setup again)
    if (first_begin_of_the_frame)
    {
        window->Active = true;
        window->OrderWithinParent = 0;
        window->BeginCount = 0;
        window->ClipRect = ImVec4(-FLT_MAX,-FLT_MAX,+FLT_MAX,+FLT_MAX);
        window->LastFrameActive = current_frame;
        window->IDStack.resize(1);

        // Clear draw list, setup texture, outer clipping rectangle
        window->DrawList->Clear();
        window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
        ImRect fullscreen_rect(GetVisibleRect());
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_ComboBox|ImGuiWindowFlags_Popup)))
            PushClipRect(parent_window->ClipRect.Min, parent_window->ClipRect.Max, true);
        else
            PushClipRect(fullscreen_rect.Min, fullscreen_rect.Max, true);

        if (!window_was_active)
        {
            // Popup first latch mouse position, will position itself when it appears next frame
            window->AutoPosLastDirection = -1;
            if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api)
                window->PosFloat = g.IO.MousePos;
        }

        // Collapse window by double-clicking on title bar
        // At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
        if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
        {
            ImRect title_bar_rect = window->TitleBarRect();
            if (g.HoveredWindow == window && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
            {
                window->Collapsed = !window->Collapsed;
                if (!(flags & ImGuiWindowFlags_NoSavedSettings))
                    MarkIniSettingsDirty();
                FocusWindow(window);
            }
        }
        else
        {
            window->Collapsed = false;
        }

        // SIZE

        // Save contents size from last frame for auto-fitting (unless explicitly specified)
        window->SizeContents.x = (float)(int)((window->SizeContentsExplicit.x != 0.0f) ? window->SizeContentsExplicit.x : ((window_is_new ? 0.0f : window->DC.CursorMaxPos.x - window->Pos.x) + window->Scroll.x));
        window->SizeContents.y = (float)(int)((window->SizeContentsExplicit.y != 0.0f) ? window->SizeContentsExplicit.y : ((window_is_new ? 0.0f : window->DC.CursorMaxPos.y - window->Pos.y) + window->Scroll.y));

        // Hide popup/tooltip window when first appearing while we measure size (because we recycle them)
        if (window->HiddenFrames > 0)
            window->HiddenFrames--;
        if ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0 && !window_was_active)
        {
            window->HiddenFrames = 1;
            if (flags & ImGuiWindowFlags_AlwaysAutoResize)
            {
                if (!window_size_set_by_api)
                    window->Size = window->SizeFull = ImVec2(0.f, 0.f);
                window->SizeContents = ImVec2(0.f, 0.f);
            }
        }

        // Lock window padding so that altering the ShowBorders flag for children doesn't have side-effects.
        window->WindowPadding = ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_ComboBox | ImGuiWindowFlags_Popup))) ? ImVec2(0,0) : style.WindowPadding;

        // Calculate auto-fit size
        ImVec2 size_auto_fit;
        if ((flags & ImGuiWindowFlags_Tooltip) != 0)
        {
            // Tooltip always resize. We keep the spacing symmetric on both axises for aesthetic purpose.
            size_auto_fit = window->SizeContents + window->WindowPadding - ImVec2(0.0f, style.ItemSpacing.y);
        }
        else
        {
            size_auto_fit = ImClamp(window->SizeContents + window->WindowPadding, style.WindowMinSize, ImMax(style.WindowMinSize, g.IO.DisplaySize - g.Style.DisplaySafeAreaPadding));

            // Handling case of auto fit window not fitting in screen on one axis, we are growing auto fit size on the other axis to compensate for expected scrollbar. FIXME: Might turn bigger than DisplaySize-WindowPadding.
            if (size_auto_fit.x < window->SizeContents.x && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar))
                size_auto_fit.y += style.ScrollbarSize;
            if (size_auto_fit.y < window->SizeContents.y && !(flags & ImGuiWindowFlags_NoScrollbar))
                size_auto_fit.x += style.ScrollbarSize;
            size_auto_fit.y = ImMax(size_auto_fit.y - style.ItemSpacing.y, 0.0f);
        }

        // Handle automatic resize
        if (window->Collapsed)
        {
            // We still process initial auto-fit on collapsed windows to get a window width,
            // But otherwise we don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
            if (window->AutoFitFramesX > 0)
                window->SizeFull.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
            if (window->AutoFitFramesY > 0)
                window->SizeFull.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
        }
        else
        {
            if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window_size_set_by_api)
            {
                window->SizeFull = size_auto_fit;
            }
            else if ((window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0) && !window_size_set_by_api)
            {
                // Auto-fit only grows during the first few frames
                if (window->AutoFitFramesX > 0)
                    window->SizeFull.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
                if (window->AutoFitFramesY > 0)
                    window->SizeFull.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
                if (!(flags & ImGuiWindowFlags_NoSavedSettings))
                    MarkIniSettingsDirty();
            }
        }

        // Apply minimum/maximum window size constraints and final size
        ApplySizeFullWithConstraint(window, window->SizeFull);
        window->Size = window->Collapsed ? window->TitleBarRect().GetSize() : window->SizeFull;

        // POSITION

        // Position child window
        if (flags & ImGuiWindowFlags_ChildWindow)
        {
            window->OrderWithinParent = parent_window->DC.ChildWindows.Size;
            parent_window->DC.ChildWindows.push_back(window);
        }
        if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup))
        {
            window->Pos = window->PosFloat = parent_window->DC.CursorPos;
            window->Size = window->SizeFull = size_on_first_use; // NB: argument name 'size_on_first_use' misleading here, it's really just 'size' as provided by user passed via BeginChild()->Begin().
        }

        bool window_pos_center = false;
        window_pos_center |= (window->SetWindowPosCenterWanted && window->HiddenFrames == 0);
        window_pos_center |= ((flags & ImGuiWindowFlags_Modal) && !window_pos_set_by_api && window_appearing_after_being_hidden);
        if (window_pos_center)
        {
            // Center (any sort of window)
            SetWindowPos(window, ImMax(style.DisplaySafeAreaPadding, fullscreen_rect.GetCenter() - window->SizeFull * 0.5f), 0);
        }
        else if (flags & ImGuiWindowFlags_ChildMenu)
        {
            // Child menus typically request _any_ position within the parent menu item, and then our FindBestPopupWindowPos() function will move the new menu outside the parent bounds.
            // This is how we end up with child menus appearing (most-commonly) on the right of the parent menu.
            IM_ASSERT(window_pos_set_by_api);
            float horizontal_overlap = style.ItemSpacing.x; // We want some overlap to convey the relative depth of each popup (currently the amount of overlap it is hard-coded to style.ItemSpacing.x, may need to introduce another style value).
            ImRect rect_to_avoid;
            if (parent_window->DC.MenuBarAppending)
                rect_to_avoid = ImRect(-FLT_MAX, parent_window->Pos.y + parent_window->TitleBarHeight(), FLT_MAX, parent_window->Pos.y + parent_window->TitleBarHeight() + parent_window->MenuBarHeight());
            else
                rect_to_avoid = ImRect(parent_window->Pos.x + horizontal_overlap, -FLT_MAX, parent_window->Pos.x + parent_window->Size.x - horizontal_overlap - parent_window->ScrollbarSizes.x, FLT_MAX);
            window->PosFloat = FindBestPopupWindowPos(window->PosFloat, window->Size, &window->AutoPosLastDirection, rect_to_avoid);
        }
        else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_appearing_after_being_hidden)
        {
            ImRect rect_to_avoid(window->PosFloat.x - 1, window->PosFloat.y - 1, window->PosFloat.x + 1, window->PosFloat.y + 1);
            window->PosFloat = FindBestPopupWindowPos(window->PosFloat, window->Size, &window->AutoPosLastDirection, rect_to_avoid);
        }

        // Position tooltip (always follows mouse)
        if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api)
        {
            ImRect rect_to_avoid(g.IO.MousePos.x - 16, g.IO.MousePos.y - 8, g.IO.MousePos.x + 24, g.IO.MousePos.y + 24); // FIXME: Completely hard-coded. Perhaps center on cursor hit-point instead?
            window->PosFloat = FindBestPopupWindowPos(g.IO.MousePos, window->Size, &window->AutoPosLastDirection, rect_to_avoid);
            if (window->AutoPosLastDirection == -1)
                window->PosFloat = g.IO.MousePos + ImVec2(2,2); // If there's not enough room, for tooltip we prefer avoiding the cursor at all cost even if it means that part of the tooltip won't be visible.
        }

        // Clamp position so it stays visible
        if (!(flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Tooltip))
        {
            if (!window_pos_set_by_api && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f) // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
            {
                ImVec2 padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
                window->PosFloat = ImMax(window->PosFloat + window->Size, padding) - window->Size;
                window->PosFloat = ImMin(window->PosFloat, g.IO.DisplaySize - padding);
            }
        }
        window->Pos = ImVec2((float)(int)window->PosFloat.x, (float)(int)window->PosFloat.y);

        // Default item width. Make it proportional to window size if window manually resizes
        if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
            window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
        else
            window->ItemWidthDefault = (float)(int)(g.FontSize * 16.0f);

        // Prepare for focus requests
        window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext == INT_MAX || window->FocusIdxAllCounter == -1) ? INT_MAX : (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter+1)) % (window->FocusIdxAllCounter+1);
        window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext == INT_MAX || window->FocusIdxTabCounter == -1) ? INT_MAX : (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter+1)) % (window->FocusIdxTabCounter+1);
        window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
        window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = INT_MAX;

        // Apply scrolling
        if (window->ScrollTarget.x < FLT_MAX)
        {
            window->Scroll.x = window->ScrollTarget.x;
            window->ScrollTarget.x = FLT_MAX;
        }
        if (window->ScrollTarget.y < FLT_MAX)
        {
            float center_ratio = window->ScrollTargetCenterRatio.y;
            window->Scroll.y = window->ScrollTarget.y - ((1.0f - center_ratio) * (window->TitleBarHeight() + window->MenuBarHeight())) - (center_ratio * window->SizeFull.y);
            window->ScrollTarget.y = FLT_MAX;
        }
        window->Scroll = ImMax(window->Scroll, ImVec2(0.0f, 0.0f));
        if (!window->Collapsed && !window->SkipItems)
            window->Scroll = ImMin(window->Scroll, ImMax(ImVec2(0.0f, 0.0f), window->SizeContents - window->SizeFull + window->ScrollbarSizes));

        // Modal window darkens what is behind them
        if ((flags & ImGuiWindowFlags_Modal) != 0 && window == GetFrontMostModalRootWindow())
            window->DrawList->AddRectFilled(fullscreen_rect.Min, fullscreen_rect.Max, GetColorU32(ImGuiCol_ModalWindowDarkening, g.ModalWindowDarkeningRatio));

        // Draw window + handle manual resize
        ImRect title_bar_rect = window->TitleBarRect();
        const float window_rounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildWindowRounding : style.WindowRounding;
        if (window->Collapsed)
        {
            // Draw title bar only
            RenderFrame(title_bar_rect.GetTL(), title_bar_rect.GetBR(),  GetColorU32(ImGuiCol_TitleBgCollapsed), true, window_rounding);
        }
        else
        {
            ImU32 resize_col = 0;
            const float resize_corner_size = ImMax(g.FontSize * 1.35f, window_rounding + 1.0f + g.FontSize * 0.2f);
            if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && !(flags & ImGuiWindowFlags_NoResize))
            {
                // Manual resize
                const ImVec2 br = window->Rect().GetBR();
                const ImRect resize_rect(br - ImVec2(resize_corner_size * 0.75f, resize_corner_size * 0.75f), br);
                const ImGuiID resize_id = window->GetID("#RESIZE");
                bool hovered, held;
                ButtonBehavior(resize_rect, resize_id, &hovered, &held, ImGuiButtonFlags_FlattenChilds);
                resize_col = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);

                if (hovered || held)
                    g.MouseCursor = ImGuiMouseCursor_ResizeNWSE;

                if (g.HoveredWindow == window && held && g.IO.MouseDoubleClicked[0])
                {
                    // Manual auto-fit when double-clicking
                    ApplySizeFullWithConstraint(window, size_auto_fit);
                    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
                        MarkIniSettingsDirty();
                    ClearActiveID();
                }
                else if (held)
                {
                    // We don't use an incremental MouseDelta but rather compute an absolute target size based on mouse position
                    ApplySizeFullWithConstraint(window, (g.IO.MousePos - g.ActiveIdClickOffset + resize_rect.GetSize()) - window->Pos);
                    if (!(flags & ImGuiWindowFlags_NoSavedSettings))
                        MarkIniSettingsDirty();
                }

                window->Size = window->SizeFull;
                title_bar_rect = window->TitleBarRect();
            }

            // Scrollbars
            window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((window->SizeContents.y > window->Size.y + style.ItemSpacing.y) && !(flags & ImGuiWindowFlags_NoScrollbar));
            window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((window->SizeContents.x > window->Size.x - (window->ScrollbarY ? style.ScrollbarSize : 0.0f) - window->WindowPadding.x) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
            window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
            window->BorderSize = (flags & ImGuiWindowFlags_ShowBorders) ? 1.0f : 0.0f;

            // Window background, Default Alpha
            ImGuiCol bg_color_idx = ImGuiCol_WindowBg;
            if ((flags & ImGuiWindowFlags_ComboBox) != 0)
                bg_color_idx = ImGuiCol_ComboBg;
            else if ((flags & ImGuiWindowFlags_Tooltip) != 0 || (flags & ImGuiWindowFlags_Popup) != 0)
                bg_color_idx = ImGuiCol_PopupBg;
            else if ((flags & ImGuiWindowFlags_ChildWindow) != 0)
                bg_color_idx = ImGuiCol_ChildWindowBg;
            ImVec4 bg_color = style.Colors[bg_color_idx];
            if (bg_alpha >= 0.0f)
                bg_color.w = bg_alpha;
            bg_color.w *= style.Alpha;
            if (bg_color.w > 0.0f)
                window->DrawList->AddRectFilled(window->Pos+ImVec2(0,window->TitleBarHeight()), window->Pos+window->Size, ColorConvertFloat4ToU32(bg_color), window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImGuiCorner_All : ImGuiCorner_BottomLeft|ImGuiCorner_BottomRight);

            // Title bar
            if (!(flags & ImGuiWindowFlags_NoTitleBar))
                window->DrawList->AddRectFilled(title_bar_rect.GetTL(), title_bar_rect.GetBR(), GetColorU32((g.FocusedWindow && window->RootNonPopupWindow == g.FocusedWindow->RootNonPopupWindow) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg), window_rounding, ImGuiCorner_TopLeft|ImGuiCorner_TopRight);

            // Menu bar
            if (flags & ImGuiWindowFlags_MenuBar)
            {
                ImRect menu_bar_rect = window->MenuBarRect();
                if (flags & ImGuiWindowFlags_ShowBorders)
                    window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border));
                window->DrawList->AddRectFilled(menu_bar_rect.GetTL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImGuiCorner_TopLeft|ImGuiCorner_TopRight);
            }

            // Scrollbars
            if (window->ScrollbarX)
                Scrollbar(window, true);
            if (window->ScrollbarY)
                Scrollbar(window, false);

            // Render resize grip
            // (after the input handling so we don't have a frame of latency)
            if (!(flags & ImGuiWindowFlags_NoResize))
            {
                const ImVec2 br = window->Rect().GetBR();
                window->DrawList->PathLineTo(br + ImVec2(-resize_corner_size, -window->BorderSize));
                window->DrawList->PathLineTo(br + ImVec2(-window->BorderSize, -resize_corner_size));
                window->DrawList->PathArcToFast(ImVec2(br.x - window_rounding - window->BorderSize, br.y - window_rounding - window->BorderSize), window_rounding, 0, 3);
                window->DrawList->PathFillConvex(resize_col);
            }

            // Borders
            if (flags & ImGuiWindowFlags_ShowBorders)
            {
                window->DrawList->AddRect(window->Pos+ImVec2(1,1), window->Pos+window->Size+ImVec2(1,1), GetColorU32(ImGuiCol_BorderShadow), window_rounding);
                window->DrawList->AddRect(window->Pos, window->Pos+window->Size, GetColorU32(ImGuiCol_Border), window_rounding);
                if (!(flags & ImGuiWindowFlags_NoTitleBar))
                    window->DrawList->AddLine(title_bar_rect.GetBL()+ImVec2(1,0), title_bar_rect.GetBR()-ImVec2(1,0), GetColorU32(ImGuiCol_Border));
            }
        }

        // Update ContentsRegionMax. All the variable it depends on are set above in this function.
        window->ContentsRegionRect.Min.x = -window->Scroll.x + window->WindowPadding.x;
        window->ContentsRegionRect.Min.y = -window->Scroll.y + window->WindowPadding.y + window->TitleBarHeight() + window->MenuBarHeight();
        window->ContentsRegionRect.Max.x = -window->Scroll.x - window->WindowPadding.x + (window->SizeContentsExplicit.x != 0.0f ? window->SizeContentsExplicit.x : (window->Size.x - window->ScrollbarSizes.x));
        window->ContentsRegionRect.Max.y = -window->Scroll.y - window->WindowPadding.y + (window->SizeContentsExplicit.y != 0.0f ? window->SizeContentsExplicit.y : (window->Size.y - window->ScrollbarSizes.y));

        // Setup drawing context
        window->DC.IndentX = 0.0f + window->WindowPadding.x - window->Scroll.x;
        window->DC.GroupOffsetX = 0.0f;
        window->DC.ColumnsOffsetX = 0.0f;
        window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.IndentX + window->DC.ColumnsOffsetX, window->TitleBarHeight() + window->MenuBarHeight() + window->WindowPadding.y - window->Scroll.y);
        window->DC.CursorPos = window->DC.CursorStartPos;
        window->DC.CursorPosPrevLine = window->DC.CursorPos;
        window->DC.CursorMaxPos = window->DC.CursorStartPos;
        window->DC.CurrentLineHeight = window->DC.PrevLineHeight = 0.0f;
        window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
        window->DC.MenuBarAppending = false;
        window->DC.MenuBarOffsetX = ImMax(window->WindowPadding.x, style.ItemSpacing.x);
        window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
        window->DC.ChildWindows.resize(0);
        window->DC.LayoutType = ImGuiLayoutType_Vertical;
        window->DC.ItemWidth = window->ItemWidthDefault;
        window->DC.TextWrapPos = -1.0f; // disabled
        window->DC.AllowKeyboardFocus = true;
        window->DC.ButtonRepeat = false;
        window->DC.ItemWidthStack.resize(0);
        window->DC.AllowKeyboardFocusStack.resize(0);
        window->DC.ButtonRepeatStack.resize(0);
        window->DC.TextWrapPosStack.resize(0);
        window->DC.ColumnsCurrent = 0;
        window->DC.ColumnsCount = 1;
        window->DC.ColumnsStartPosY = window->DC.CursorPos.y;
        window->DC.ColumnsCellMinY = window->DC.ColumnsCellMaxY = window->DC.ColumnsStartPosY;
        window->DC.TreeDepth = 0;
        window->DC.StateStorage = &window->StateStorage;
        window->DC.GroupStack.resize(0);
        window->DC.ColorEditMode = ImGuiColorEditMode_UserSelect;
        window->MenuColumns.Update(3, style.ItemSpacing.x, !window_was_active);

        if (window->AutoFitFramesX > 0)
            window->AutoFitFramesX--;
        if (window->AutoFitFramesY > 0)
            window->AutoFitFramesY--;

        // New windows appears in front (we need to do that AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
        if (!window_was_active && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
            if (!(flags & (ImGuiWindowFlags_ChildWindow|ImGuiWindowFlags_Tooltip)) || (flags & ImGuiWindowFlags_Popup))
                FocusWindow(window);

        // Title bar
        if (!(flags & ImGuiWindowFlags_NoTitleBar))
        {
            if (p_open != NULL)
            {
                const float pad = 3.0f;
                const float rad = (window->TitleBarHeight() - pad*2.0f) * 0.5f;

                if (CloseButtonOverride(window->GetID("#CLOSE"), window->Rect().GetTR() + ImVec2(-pad - rad, pad + rad), rad))
                    *p_open = false;
            }

            const ImVec2 text_size = CalcTextSize(name, NULL, true);
            //if (!(flags & ImGuiWindowFlags_NoCollapse))
            //    RenderCollapseTriangle(window->Pos + style.FramePadding, !window->Collapsed, 1.0f);

            ImVec2 text_min = window->Pos;
            ImVec2 text_max = window->Pos + ImVec2(window->Size.x, style.FramePadding.y*2 + text_size.y);
            ImRect clip_rect;
            clip_rect.Max = ImVec2(window->Pos.x + window->Size.x - (p_open ? title_bar_rect.GetHeight() - 3 : style.FramePadding.x), text_max.y); // Match the size of CloseWindowButton()
            float pad_left = (flags & ImGuiWindowFlags_NoCollapse) == 0 ? (style.FramePadding.x) : style.FramePadding.x;
            float pad_right = (p_open != NULL) ? (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x) : style.FramePadding.x;
            if (style.WindowTitleAlign.x > 0.0f) pad_right = ImLerp(pad_right, pad_left, style.WindowTitleAlign.x);
            text_min.x += pad_left;
            text_max.x -= pad_right;
            clip_rect.Min = ImVec2(text_min.x, window->Pos.y);
            RenderTextClipped(text_min, text_max, name, NULL, &text_size, style.WindowTitleAlign, &clip_rect);
        }

        // Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
        window->WindowRectClipped = window->Rect();
        window->WindowRectClipped.Clip(window->ClipRect);

        // Pressing CTRL+C while holding on a window copy its content to the clipboard
        // This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
        // Maybe we can support CTRL+C on every element?
        /*
        if (g.ActiveId == move_id)
            if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
                ImGui::LogToClipboard();
        */
    }

    // Inner clipping rectangle
    // We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
    // Note that if our window is collapsed we will end up with a null clipping rectangle which is the correct behavior.
    const ImRect title_bar_rect = window->TitleBarRect();
    const float border_size = window->BorderSize;
    ImRect clip_rect; // Force round to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
    clip_rect.Min.x = ImFloor(0.5f + title_bar_rect.Min.x + ImMax(border_size, ImFloor(window->WindowPadding.x*0.5f)));
    clip_rect.Min.y = ImFloor(0.5f + title_bar_rect.Max.y + window->MenuBarHeight() + border_size);
    clip_rect.Max.x = ImFloor(0.5f + window->Pos.x + window->Size.x - window->ScrollbarSizes.x - ImMax(border_size, ImFloor(window->WindowPadding.x*0.5f)));
    clip_rect.Max.y = ImFloor(0.5f + window->Pos.y + window->Size.y - window->ScrollbarSizes.y - border_size);
    PushClipRect(clip_rect.Min, clip_rect.Max, true);

    // Clear 'accessed' flag last thing
    if (first_begin_of_the_frame)
        window->Accessed = false;
    window->BeginCount++;
    g.SetNextWindowSizeConstraint = false;

    // Child window can be out of sight and have "negative" clip windows.
    // Mark them as collapsed so commands are skipped earlier (we can't manually collapse because they have no title bar).
    if (flags & ImGuiWindowFlags_ChildWindow)
    {
        IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
        window->Collapsed = parent_window && parent_window->Collapsed;

        if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
            window->Collapsed |= (window->WindowRectClipped.Min.x >= window->WindowRectClipped.Max.x || window->WindowRectClipped.Min.y >= window->WindowRectClipped.Max.y);

        // We also hide the window from rendering because we've already added its border to the command list.
        // (we could perform the check earlier in the function but it is simpler at this point)
        if (window->Collapsed)
            window->Active = false;
    }
    if (style.Alpha <= 0.0f)
        window->Active = false;

    // Return false if we don't intend to display anything to allow user to perform an early out optimization
    window->SkipItems = (window->Collapsed || !window->Active) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0;
    return !window->SkipItems;
}

#endif
