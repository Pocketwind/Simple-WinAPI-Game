#include"inc/fmod.hpp"
#include"rapidjson/document.h"
#include"rapidjson/writer.h"
#include"rapidjson/stringbuffer.h"
#include<Windows.h>
#include<tchar.h>
#include <stdlib.h>
#include "resource.h" 
#include <stdio.h>
#include<vector>
#include<string>

#pragma warning(disable:4996)

using namespace rapidjson;
using namespace std;
using namespace FMOD;

System* fmod_system;
Sound* fmod_sound[4];
Channel* fmod_channel;
FMOD_RESULT fmod_result;
void* extradrivedata;

TCHAR name[100];
int score;

enum State { run_right = 100, run_left, dash_left, dash_right, idle_left, idle_right, attack_right, attack_left, jump_right, jump_left, hurt_right, hurt_left };
enum Enemy_State { enemy_run_right = 200, enemy_run_left, enemy_hurt_right, enemy_hurt_left };
enum Timer { enemy_appear = 300, frame_run_timer, frame_idle_timer, frame_jump_timer, frame_attack_timer, frame_hurt_timer, frame_dash_timer, enemy_run_timer, global_timer, blink_timer };
enum SoundNum { enemy_entered = 0, hit, running, whip };

HINSTANCE hInst;
HWND hMainWnd;
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlg_Name(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
bool isInSquare(int bigx1, int bigy1, int bigx2, int bigy2, int smallx1, int smally1, int smallx2, int smally2);
void PlaySfx(SoundNum n);
void sort_rank(vector<int>& score, vector<string>& name);



void ReleaseFmod()
{
	fmod_system->release();
	fmod_system->close();
}

void InitSound()
{
	System_Create(&fmod_system);
	fmod_system->init(10, FMOD_INIT_NORMAL, extradrivedata);
	fmod_system->createSound("sfx\\enemy_entered.wav", FMOD_LOOP_OFF, 0, &fmod_sound[0]);
	fmod_system->createSound("sfx\\hit.wav", FMOD_LOOP_OFF, 0, &fmod_sound[1]);
	fmod_system->createSound("sfx\\running.wav", FMOD_LOOP_OFF, 0, &fmod_sound[2]);
	fmod_system->createSound("sfx\\whip.wav", FMOD_LOOP_OFF, 0, &fmod_sound[3]);
	fmod_system->update();
}

bool isInSquare(int bigx1, int bigy1, int bigx2, int bigy2, int smallx1, int smally1,int smallx2,int smally2)
{
	if (bigx1<=smallx1&&bigx2>=smallx1||bigx1<=smallx2&&bigx2>=smallx2)
		return true;
	else
		return false;
}

void PlaySfx(SoundNum n)
{
	fmod_system->playSound(fmod_sound[n], 0, false, &fmod_channel);
	fmod_channel->setVolume(0.1);
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASS WndClass;

	hInst = hInstance;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.lpszClassName = _T("mywindow");
	RegisterClass(&WndClass);

	hwnd = CreateWindow(_T("mywindow"), _T("Slasher!"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, NULL, NULL, hInstance, NULL);

	hMainWnd = hwnd;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		fmod_system->update();
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}



#define ENEMY_SIZE 5
#define MAX_HEALTH 5
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR temp[20];
	vector<int> rank_score;
	vector<string> rank_name;
	HDC hdc, mem1dc, mem2dc;
	PAINTSTRUCT ps;
	static HBITMAP backBitmap, frame_background, frame_background1, frame_background2, frame_tileset;
	static HBITMAP mask_background1, mask_background2, mask_tileset;
	static HBITMAP frame_idle_right, frame_run_right, frame_attack_right, frame_jump_right, frame_hurt_right;
	static HBITMAP frame_idle_left, frame_run_left, frame_attack_left, frame_jump_left, frame_hurt_left;
	static HBITMAP mask_idle_right, mask_run_right, mask_attack_right, mask_jump_right, mask_jump_left, mask_hurt_right;
	static HBITMAP mask_idle_left, mask_run_left, mask_attack_left, mask_hurt_left;
	static HBITMAP frame_enemy_run_left, frame_enemy_run_right;
	static int frame_left, frame_right, x, y, frame_first_right[ENEMY_SIZE], frame_first_left[ENEMY_SIZE];
	static int enemy_frame_left[ENEMY_SIZE], enemy_frame_right[ENEMY_SIZE], enemy_x[ENEMY_SIZE], enemy_y[ENEMY_SIZE];
	static int time, health;
	static State state;
	static Enemy_State enemy_state[ENEMY_SIZE];
	static bool enemy_alive[ENEMY_SIZE];
	static RECT rt;
	static bool hitbox, is_paused, is_end, blink;
	switch (iMsg)
	{
	case WM_CREATE:
		InitSound();

		health = MAX_HEALTH;
		time = 60;
		GetClientRect(hwnd, &rt);
		x = 100;
		enemy_y[0] = enemy_y[1] = enemy_y[2] = enemy_y[3] = enemy_y[4] = y = 510;
		state = idle_right;
		SetTimer(hwnd, frame_run_timer, 30, NULL);
		SetTimer(hwnd, frame_idle_timer, 50, NULL);
		SetTimer(hwnd, frame_attack_timer, 40, NULL);
		SetTimer(hwnd, frame_dash_timer, 25, NULL);
		SetTimer(hwnd, frame_hurt_timer, 50, NULL);
		SetTimer(hwnd, enemy_run_timer, 50, NULL);
		SetTimer(hwnd, enemy_appear, 3000, NULL);
		SetTimer(hwnd, global_timer, 1000, NULL);
		SetTimer(hwnd, blink_timer, 500, NULL);

		frame_background = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BACKGROUND));
		frame_background1 = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BACKGROUND1));
		frame_background2 = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BACKGROUND2));
		frame_tileset = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TILESET));
		mask_tileset = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TILESET_MASK));
		mask_background1 = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BACKGROUND1_MASK));
		mask_background2 = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BACKGROUND2_MASK));


		frame_idle_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_IDLELEFT));
		frame_idle_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_IDLERIGHT));
		frame_run_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RUNRIGHT));
		frame_run_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RUNLEFT));
		frame_attack_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ATTACKLEFT));
		frame_attack_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ATTACKRIGHT));
		frame_hurt_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_HURTLEFT));
		frame_hurt_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_HURTRIGHT));
		frame_jump_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_JUMPLEFT));
		frame_jump_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_JUMPRIGHT));

		mask_idle_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_IDLELEFTMASK));
		mask_idle_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_IDLERIGHTMASK));
		mask_run_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RUNRIGHTMASK));
		mask_run_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RUNLEFTMASK));
		mask_attack_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ATTACKLEFTMASK));
		mask_attack_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ATTACKRIGHTMASK));
		mask_hurt_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_HURTLEFTMASK));
		mask_hurt_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_HURTRIGHTMASK));
		mask_jump_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_JUMPLEFTMASK));
		mask_jump_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_JUMPRIGHTMASK));

		frame_enemy_run_left = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ENEMY_RUN_LEFT));
		frame_enemy_run_right = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ENEMY_RUN_LEFT));

		break;
	case WM_DESTROY:
		KillTimer(hwnd, frame_run_timer);
		KillTimer(hwnd, frame_idle_timer);
		KillTimer(hwnd, frame_attack_timer);
		KillTimer(hwnd, frame_dash_timer);
		KillTimer(hwnd, frame_hurt_timer);
		KillTimer(hwnd, enemy_run_timer);
		KillTimer(hwnd, enemy_appear);
		KillTimer(hwnd, global_timer);
		KillTimer(hwnd, blink_timer);
		ReleaseFmod();
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_RESTART:
			is_end = is_paused = false;
			score = 0;
			health = MAX_HEALTH;
			time = 60;
			GetClientRect(hwnd, &rt);
			x = 100;
			enemy_y[0] = enemy_y[1] = enemy_y[2] = enemy_y[3] = enemy_y[4] = y = 510;
			enemy_alive[0] = enemy_alive[1] = enemy_alive[2] = enemy_alive[3] = enemy_alive[4] = false;
			state = idle_right;
			break;
		case ID_DEBUG_HITBOX:
			hitbox = 1 - hitbox;
			break;
		}
		break;
	case WM_LBUTTONDOWN:
		if (!is_paused)
		{
			if (state == run_left || state == idle_left || state == attack_left)
			{
				state = attack_left;
				for (int i = 0; i < ENEMY_SIZE; ++i)
				{
					if (enemy_alive[i])
					{
						if (isInSquare(enemy_x[i] + 60, enemy_y[i] + 40, enemy_x[i] + 100, enemy_y[i] + 120, x + 40, y + 80, x + 60, y + 100))
						{
							//PlaySfx(".\\sfx\\hit.wav");
							PlaySfx(whip);
							score += 100;
							if (enemy_state[i] == enemy_run_right)
							{
								enemy_state[i] = enemy_hurt_right;
								enemy_frame_right[i] = 0;
							}
							else if (enemy_state[i] == enemy_run_left)
							{
								enemy_state[i] = enemy_hurt_left;
								enemy_frame_left[i] = 6;
							}
						}
					}
				}
			}
			else if (state == run_right || state == idle_right || state == attack_right)
			{
				state = attack_right;
				for (int i = 0; i < ENEMY_SIZE; ++i)
				{
					if (enemy_alive[i])
					{
						if (isInSquare(enemy_x[i] + 60, enemy_y[i] + 40, enemy_x[i] + 100, enemy_y[i] + 120, x + 100, y + 80, x + 120, y + 100))
						{
							//PlaySfx(".\\sfx\\hit.wav");
							PlaySfx(whip);
							score += 10;
							if (enemy_state[i] == enemy_run_right)
							{
								enemy_state[i] = enemy_hurt_right;
								enemy_frame_right[i] = 0;
							}
							else if (enemy_state[i] == enemy_run_left)
							{
								enemy_state[i] = enemy_hurt_left;
								enemy_frame_left[i] = 6;
							}
						}
					}
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		if (!is_paused)
		{
			if (state == attack_left)
			{
				//frame_left = 18;
				state = idle_left;
			}
			else if (state == attack_right)
			{
				//frame_right = 0;
				state = idle_right;
			}
		}
		break;
		
	case WM_KEYUP:
		if (!is_paused)
		{
			//frame_left = 18;
			//frame_right = 0;
			if (state == run_left || state == dash_left || state == attack_left)
			{
				state = idle_left;
			}
			else if (state == run_right || state == dash_right || state == attack_right)
			{
				state = idle_right;
			}
		}
		break;
	case WM_CHAR:
		if (!is_paused)
		{
			if (!(state == hurt_left || state == hurt_right))
			{
				switch (wParam)
				{
				case 'a':
					//PlaySfx(running);
					state = run_left;
					break;
				case 'd':
					state = run_right;
					break;
				}
			}
		}
		break;
	case WM_KEYDOWN:

		switch (wParam)
		{
		case VK_SHIFT:
			if (!is_paused)
			{
				if (state == idle_left)
					state = dash_left;
				else if (state == idle_right)
					state = dash_right;
			}
			break;
		case VK_SPACE:
			is_paused = 1 - is_paused;
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}
		break;

	case WM_TIMER:
		if (wParam == blink_timer)
		{
			blink = 1 - blink;
			InvalidateRgn(hwnd, NULL, FALSE);
		}


		if (!is_paused)
		{
			for (int i = 0; i < ENEMY_SIZE; ++i)
			{
				if (enemy_alive[i])
				{
					if (isInSquare(x + 60, y + 40, x + 100, y + 120, enemy_x[i] + 60, enemy_y[i] + 40, enemy_x[i] + 100, enemy_y[i] + 120))
					{
						PlaySfx(hit);
						if (state == run_left || state == dash_left || state == idle_left)
						{
							state = hurt_left;
							frame_left = 6;
							x += 50;
						}
						else if (state == run_right || state == dash_right || state == idle_right)
						{
							state = hurt_right;
							frame_right = 0;
							x -= 50;
						}
						--health;
						if (health <= 0)
						{
							is_paused = true;
							is_end = true;
							DialogBox(hInst, MAKEINTRESOURCE(IDD_NAME), hwnd, (DLGPROC)&Dlg_Name);
							InvalidateRgn(hwnd, NULL, FALSE);
						}
					}
				}
			}

			switch (LOWORD(wParam))
			{
			case enemy_appear:
				for (int i = 0; i < ENEMY_SIZE; ++i)
				{
					if (enemy_alive[i] == false)
					{
						//PlaySfx(".\\sfx\\enemy_entered.wav");
						PlaySfx(enemy_entered);
						enemy_alive[i] = true;
						enemy_state[i] = rand() % 2 ? enemy_run_right : enemy_run_left;
						switch (enemy_state[i])
						{
						case enemy_run_right:
							enemy_x[i] = -200;
							break;
						case enemy_run_left:
							enemy_x[i] = rt.right;
							break;
						}
						break;
					}
				}
				break;
			case global_timer:
				score += 30;
				if (time == 0)
				{
					is_end = true;
					is_paused = true;
					break;
				}
				time -= 1;
				break;
			case frame_run_timer:
				switch (state)				//player
				{
				case run_right:
					++frame_right;
					frame_right = frame_right % 24;
					x += 5;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				case run_left:
					--frame_left;
					frame_left = (24 + frame_left) % 24;
					x -= 5;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				}
				break;
			case frame_dash_timer:
				switch (state)
				{
				case dash_right:
					++frame_right;
					frame_right = frame_right % 24;
					x += 8;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				case dash_left:
					--frame_left;
					frame_left = (24 + frame_left) % 24;
					x -= 8;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				}
				break;
			case frame_idle_timer:
				switch (state)
				{
				case idle_right:
					++frame_right;
					frame_right = frame_right % 18;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				case idle_left:
					--frame_left;
					frame_left = (18 + frame_left) % 18;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				}
				break;
			case frame_attack_timer:
				switch (state)
				{
				case attack_left:
					--frame_left;
					frame_left = (26 + frame_left) % 26;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				case attack_right:
					++frame_right;
					frame_right = frame_right % 26;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				}
				break;
			case frame_hurt_timer:
				switch (state)
				{
				case hurt_left:
					--frame_left;
					if (frame_left < 0)
						state = idle_left;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				case hurt_right:
					++frame_right;
					if (frame_right > 6)
						state = idle_right;
					InvalidateRgn(hwnd, NULL, FALSE);
					break;
				}
				
				for (int i = 0; i < ENEMY_SIZE; ++i)
				{
					if (enemy_alive[i])
					{
						switch (enemy_state[i])
						{
						case enemy_hurt_left:
							--enemy_frame_left[i];
							if (enemy_frame_left[i] < 0)
								enemy_alive[i] = false;
							InvalidateRgn(hwnd, NULL, FALSE);
							break;
						case enemy_hurt_right:
							++enemy_frame_right[i];
							if (enemy_frame_right[i] > 6)
								enemy_alive[i] = false;
							InvalidateRgn(hwnd, NULL, FALSE);
							break;
						}
					}
				}
				break;
			case enemy_run_timer:
				for (int i = 0; i < ENEMY_SIZE; ++i)
				{
					if (enemy_alive[i])
					{
						switch (enemy_state[i])
						{
						case enemy_run_right:
							++enemy_frame_right[i];
							enemy_x[i] += 3;
							enemy_frame_right[i] = enemy_frame_right[i] % 24;
							if (enemy_state[i] != enemy_hurt_right)
							{
								if (enemy_x[i] > x)
									enemy_state[i] = enemy_run_left;
							}
							InvalidateRgn(hwnd, NULL, FALSE);
							break;
						case enemy_run_left:
							--enemy_frame_left[i];
							enemy_x[i] -= 3;
							enemy_frame_left[i] = (24 + enemy_frame_left[i]) % 24;
							if (enemy_state[i] != enemy_hurt_left)
							{
								if (enemy_x[i] < x)
									enemy_state[i] = enemy_run_right;
							}
							InvalidateRgn(hwnd, NULL, FALSE);
							break;
						}
					}
				}
				break;
			}
		}
		break;
	case WM_SIZE:
		GetClientRect(hwnd, &rt);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		mem1dc = CreateCompatibleDC(hdc);
		mem2dc = CreateCompatibleDC(hdc);

		backBitmap = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);
		SelectObject(mem2dc, backBitmap);

		SelectObject(mem1dc, frame_background);
		StretchBlt(mem2dc, 0, 0, rt.right, rt.bottom, mem1dc, 0, 0, 288, 180, SRCCOPY);

		SelectObject(mem1dc, mask_tileset);
		StretchBlt(mem2dc, 0, 80, rt.right, rt.bottom, mem1dc, 0, 0, 288, 176, SRCAND);
		SelectObject(mem1dc, frame_tileset);
		StretchBlt(mem2dc, 0, 80, rt.right, rt.bottom, mem1dc, 0, 0, 288, 176, SRCPAINT);


		if (hitbox)
		{
			if (state == attack_right)
			{
				Rectangle(mem2dc, x + 100, y + 80, x + 120, y + 100);		//player hitbox
			}
			else if (state == attack_left)
			{
				Rectangle(mem2dc, x + 40, y + 80, x + 60, y + 100);		//player hitbox
			}

			Rectangle(mem2dc, x + 60, y + 40, x + 100, y + 120);			//player hurtbox

			//hitbox
			for (int i = 0; i < ENEMY_SIZE; ++i)
			{
				if (enemy_alive[i])
					Rectangle(mem2dc, enemy_x[i] + 60, enemy_y[i] + 40, enemy_x[i] + 100, enemy_y[i] + 120);			//enemy hitbox
			}
		}


		for (int i = 0; i < ENEMY_SIZE; ++i)				//enemy
		{
			if (enemy_alive[i])
			{
				switch (enemy_state[i])
				{
				case enemy_run_right:
					SelectObject(mem1dc, mask_run_right);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_right[i] * 80, 0, 80, 80, SRCAND);
					SelectObject(mem1dc, frame_enemy_run_right);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_right[i] * 80, 0, 80, 80, SRCPAINT);
					break;
				case enemy_run_left:
					SelectObject(mem1dc, mask_run_left);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_left[i] * 80, 0, 80, 80, SRCAND);
					SelectObject(mem1dc, frame_enemy_run_left);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_left[i] * 80, 0, 80, 80, SRCPAINT);
					break;
				case enemy_hurt_right:
					SelectObject(mem1dc, mask_hurt_right);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_right[i] * 80, 0, 80, 80, SRCAND);
					SelectObject(mem1dc, frame_hurt_right);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_right[i] * 80, 0, 80, 80, SRCPAINT);
					break;
				case enemy_hurt_left:
					SelectObject(mem1dc, mask_hurt_left);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_left[i] * 80, 0, 80, 80, SRCAND);
					SelectObject(mem1dc, frame_hurt_left);
					StretchBlt(mem2dc, enemy_x[i], enemy_y[i], 160, 160, mem1dc, enemy_frame_left[i] * 80, 0, 80, 80, SRCPAINT);
					break;
				}
			}
		}

		switch (state)		//player
		{
		case idle_right:
			SelectObject(mem1dc, mask_idle_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_idle_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCPAINT);
			break;
		case idle_left:
			SelectObject(mem1dc, mask_idle_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_idle_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCPAINT);
			break;
		case run_right:
		case dash_right:
			SelectObject(mem1dc, mask_run_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_run_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCPAINT);
			break;
		case run_left:
		case dash_left:
			SelectObject(mem1dc, mask_run_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_run_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCPAINT);
			break;
		case attack_right:
			SelectObject(mem1dc, mask_attack_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_attack_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCPAINT);
			break;
		case attack_left:
			SelectObject(mem1dc, mask_attack_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_attack_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCPAINT);
			break;
		case hurt_right:
			SelectObject(mem1dc, mask_hurt_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_hurt_right);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_right * 80, 0, 80, 80, SRCPAINT);
			break;
		case hurt_left:
			SelectObject(mem1dc, mask_hurt_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCAND);
			SelectObject(mem1dc, frame_hurt_left);
			StretchBlt(mem2dc, x, y, 160, 160, mem1dc, frame_left * 80, 0, 80, 80, SRCPAINT);
			break;
		}

		BitBlt(hdc, 0, 0, rt.right, rt.bottom, mem2dc, 0, 0, SRCCOPY);




		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(200, 200, 200));
		TCHAR text[20] = _T("");
		wsprintf(text, _T("Time : %d"), time);
		TextOut(hdc, 0, 0, text, _tcslen(text));
		wsprintf(text, _T("Score : %d"), score);
		TextOut(hdc, 0, 15, text, _tcslen(text));

		HBRUSH grey = CreateSolidBrush(RGB(100, 100, 100));
		HBRUSH lightgrey = CreateSolidBrush(RGB(200, 200, 200));
		HBRUSH red = CreateSolidBrush(RGB(255, 0, 0));
		RECT healthbar;
		if (health == 1)
		{
			healthbar = { 150,10,200,30 };
			if (blink)
				FillRect(hdc, &healthbar, red);
		}
		else
		{
			for (int i = 0; i < MAX_HEALTH; ++i)
			{
				if (i < health)
				{
					healthbar = { 150 + i * 50, 10, 200 + i * 50, 30 };
					FillRect(hdc, &healthbar, red);
				}
			}
		}
		healthbar = { 150,10,150 + 50 * MAX_HEALTH,30 };
		FrameRect(hdc, &healthbar, lightgrey);

		if (is_paused&&!is_end)
		{
			RECT font = { 0,0,600,400 };
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 0, 0));
			if (blink)
				TextOut(hdc, 600, 200, _T("Paused!"), 7);
		}
		else if (is_end)
		{
			vector<int>rank_score;
			vector<string>rank_name;

			FILE* file;
			Document doc;
			char json_str[500];
			file = fopen(".\\rank.json", "r");
			fgets(json_str, 500, file);
			fclose(file);
			doc.Parse(json_str);
			for (auto& i : doc["name"].GetArray())
			{
				rank_name.push_back(i.GetString());
			}
			for (auto& i : doc["score"].GetArray())
			{
				rank_score.push_back(i.GetInt());
			}

			RECT board = { 400,100,850,500 };
			FillRect(hdc, &board, grey);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 0, 0));
			TextOut(hdc, 600, 200, _T("Game Over!"), 9);
			SetTextColor(hdc, RGB(150, 150, 150));
			TextOut(hdc, 600, 250, _T("RANK"), 4);
			for (int i = 0; i < rank_name.size(); ++i)
			{
				int len = MultiByteToWideChar(CP_ACP, 0, rank_name[i].c_str(), strlen(rank_name[i].c_str()), NULL, NULL);
				MultiByteToWideChar(CP_ACP, 0, rank_name[i].c_str(), strlen(rank_name[i].c_str()), temp, len);
				temp[len] = NULL;
				TextOut(hdc, 450, 300 + i * 15, temp, _tcslen(temp));
				if (i == 9)
					break;
			}
			for (int i = 0; i < rank_score.size(); ++i)
			{
				_stprintf(temp, _T("%d"), rank_score[i]);
				TextOut(hdc, 700, 300 + i * 15, temp, _tcslen(temp));
				if (i == 9)
					break;
			}
		}


		DeleteObject(grey);
		DeleteObject(red);
		DeleteObject(lightgrey);
		DeleteDC(mem1dc);
		DeleteDC(mem2dc);
		EndPaint(hwnd, &ps);
		break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

BOOL CALLBACK Dlg_Name(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{
	vector<int>rank_score;
	vector<string>rank_name;

	switch (iMsg) 
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_OK:		// 버튼
			GetDlgItemText(hDlg, IDC_NAME, name, 100);
			FILE* file;
			Document doc;
			char json_str[500];
			file = fopen(".\\rank.json", "r");
			if (file == NULL)
			{
				MessageBox(hDlg, _T("rank 파일이 없습니다."), _T("error"), MB_OK);
				ReleaseFmod();
				PostQuitMessage(0);
			}
			fgets(json_str, 500, file);
			fclose(file);
			doc.Parse(json_str);
			for (auto& i : doc["name"].GetArray())
			{
				rank_name.push_back(i.GetString());
			}
			for (auto& i : doc["score"].GetArray())
			{
				rank_score.push_back(i.GetInt());
			}
			char mbname[100];
			int len = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, NULL, NULL, NULL);
			WideCharToMultiByte(CP_ACP, 0, name, -1, mbname, len, NULL, NULL);
			rank_name.push_back(mbname);
			rank_score.push_back(score);
			sort_rank(rank_score, rank_name);

			StringBuffer json;
			Writer<StringBuffer, UTF8<>> writer(json);
			writer.StartObject();
			writer.String("name");
			writer.StartArray();
			for (int i = 0; i < rank_name.size(); ++i)
			{
				writer.String(rank_name[i].c_str());
				if (i == 9)
					break;
			}
			writer.EndArray();
			writer.String("score");
			writer.StartArray();
			for (int i = 0; i < rank_score.size(); ++i)
			{
				writer.Int(rank_score[i]);
				if (i == 9)
					break;
			}
			writer.EndArray();
			writer.EndObject();
			file = fopen("rank.json", "w");
			fprintf(file, json.GetString());
			fclose(file);
			EndDialog(hDlg, 0);
			break;
		}
		break;
	}
	return 0;
}

void sort_rank(vector<int>& score, vector<string>& name)
{
	int n = score.size();
	int temp;
	string temp_name;
	for (int i = 0; i < n - 1; i++) {
		for (int j = 0; j < n - i - 1; j++) {
			if (score[j] < score[j + 1]) {
				temp = score[j];
				temp_name = name[i];
				score[j] = score[j + 1];
				name[j] = name[j + 1];
				score[j + 1] = temp;
				name[j + 1] = temp_name;
			}
		}
	}
}