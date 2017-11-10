/*
	This is free and unencumbered software released into the public domain.

	Anyone is free to copy, modify, publish, use, compile, sell, or
	distribute this software, either in source code form or as a compiled
	binary, for any purpose, commercial or non-commercial, and by any
	means.

	In jurisdictions that recognize copyright laws, the author or authors
	of this software dedicate any and all copyright interest in the
	software to the public domain. We make this dedication for the benefit
	of the public at large and to the detriment of our heirs and
	successors. We intend this dedication to be an overt act of
	relinquishment in perpetuity of all present and future rights to this
	software under copyright law.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.

	For more information, please refer to <http://unlicense.org>
*/

/*
	使い方
	#include "FixedAspectRatio.h"

	Addon::Register<FixedAspectRatio>();

	// 後からアスペクト比を変更することもできます。
	Addon::GetAddon<FixedAspectRatio>()->SetAspectRatio(16.0 / 9);
	Addon::GetAddon<FixedAspectRatio>()->SetAspectRatio(4, 3);
*/

#pragma once

#include <Siv3D.hpp>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/// <summary>
/// ウインドウのアスペクト比を固定するアドオン
/// </summary>
class FixedAspectRatio : public IAddon {
public:
	FixedAspectRatio() :
		prevRect_({0, 0, 0, 0}),
		ratio_(Window::AspectRatio()),
		prevWindowSizing_(false) {}

	virtual ~FixedAspectRatio() {
		UnhookWinEvent(hook_);
	}

	static String name() {
		return L"FixedAspectRatio";
	}

	String getName() const override {
		return name();
	}

	bool init() override {
		WCHAR path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		hWnd_ = FindWindowW(FileSystem::NormalizedPath(path).c_str(),
			Window::GetTitle().c_str());
		if (!hWnd_) {
			return false;
		}

		hook_ = SetWinEventHook(
			EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZEEND, nullptr,
			WinEventProc, GetCurrentProcessId(), 0, 0);

		if (!hook_) {
			return false;
		}

		static const DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION |
			WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;

		RECT adjustRect = {0, 0, 0, 0};
		AdjustWindowRectEx(&adjustRect, style, FALSE, WS_EX_APPWINDOW);

		xoff_ = adjustRect.right - adjustRect.left;
		yoff_ = adjustRect.bottom - adjustRect.top;

		return true;
	}

	bool update() override {
		PeekMessage(nullptr, nullptr, 0, 0, PM_REMOVE);

		if (prevWindowSizing_ && !isWindowSizing()) {
			RECT rect;
			GetWindowRect(hWnd_, &rect);

			if (rect.left != prevRect_.left || rect.right != prevRect_.right) {
				const int width = rect.right - rect.left;
				MoveWindow(hWnd_, rect.left, rect.top,
					width, int((width - xoff_) / ratio_) + yoff_, TRUE);
			} else if (rect.top != prevRect_.top || rect.bottom != prevRect_.bottom) {
				const int height = rect.bottom - rect.top;
				MoveWindow(hWnd_, rect.left, rect.top,
					int((height - yoff_) * ratio_) + xoff_, height, TRUE);
			}

			prevRect_ = rect;
		}
		prevWindowSizing_ = isWindowSizing();

		return true;
	}

	/// <summary>
	/// アスペクト比を設定します。
	/// </summary>
	/// <param name="ratio">
	/// 横幅/高さの値
	/// </param>
	void SetAspectRatio(double ratio) {
		ratio_ = ratio;
	}

	/// <summary>
	/// アスペクト比を設定します。
	/// </summary>
	/// <param name="numerator">
	/// 分子(横幅)
	/// </param>
	/// <param name="denominator">
	/// 分母(高さ)
	/// </param>
	void SetAspectRatio(double numerator, double denominator) {
		ratio_ = numerator / denominator;
	}

private:
	HWND hWnd_;
	HWINEVENTHOOK hook_;
	RECT prevRect_;
	int xoff_, yoff_;
	bool prevWindowSizing_;
	double ratio_;

	static bool& isWindowSizing() {
		static bool sizing;
		return sizing;
	}

	static void CALLBACK WinEventProc(
		HWINEVENTHOOK, DWORD event, HWND, LONG, LONG, DWORD, DWORD) {
		switch (event) {
		case EVENT_SYSTEM_MOVESIZESTART:
			isWindowSizing() = true;
			break;
		case EVENT_SYSTEM_MOVESIZEEND:
			isWindowSizing() = false;
			break;
		}
	}
};