#include <Application.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>
#include <Window.h>
#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <time.h>

enum { MSG_FRAME = 'fram' };
const int32 kPlanets = 5, kCooldown = 50, kExplosion = 67;
using namespace std;

struct Planet { float x, y, speed, size; int32 explosion; };
struct Pickup { float x, y; bool active; };

class GameView : public BView {
public:
	GameView() : BView(BRect(0, 0, 700, 520), "game", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE),
		fShipX(320), fTargetX(320), fScore(0), fLives(3), fBulletX(0), fBulletY(0),
		fCooldown(0), fBulletActive(false), fGameOver(false)
	{
		SetViewColor(3, 7, 24); srand((unsigned)time(NULL));
		for (int32 i = 0; i < kPlanets; i++) ResetPlanet(i, -80.0f * i);
		fPickup.active = false;
	}
	void AttachedToWindow() override { MakeFocus(true); }

	void Draw(BRect) override
	{
		BRect b = Bounds(); SetHighColor(3, 7, 24); FillRect(b); DrawBackground(b); DrawHud(b);
		for (int32 i = 0; i < kPlanets; i++) DrawPlanet(fPlanets[i]);
		DrawBullet(); DrawPickup(); DrawShip(b); DrawFooter(b);
		if (fGameOver) {
			DrawGameOverBackground(b);
			SetHighColor(255, 91, 119); SetFontSize(34); DrawString("MISSION FAILED", BPoint(b.Width() / 2 - 144, 220));
			SetHighColor(220, 243, 255); SetFontSize(15); DrawString("Press R to launch a new mission", BPoint(b.Width() / 2 - 127, 252));
		}
	}

	void KeyDown(const char* bytes, int32) override
	{
		if (bytes[0] == 'a' || bytes[0] == 'A') fTargetX = fTargetX > 32 ? fTargetX - 45 : 32;
		else if (bytes[0] == 'd' || bytes[0] == 'D') fTargetX = fTargetX < Bounds().right - 82 ? fTargetX + 45 : Bounds().right - 82;
		else if (bytes[0] == ' ') Fire();
		else if (bytes[0] == 'r' || bytes[0] == 'R') Restart();
	}
	void MouseDown(BPoint where) override
	{
		fTargetX = where.x - 30; if (fTargetX < 20) fTargetX = 20;
		if (fTargetX > Bounds().right - 75) fTargetX = Bounds().right - 75; MakeFocus(true);
	}

	void MessageReceived(BMessage* message) override
	{
		if (message->what != MSG_FRAME) { BView::MessageReceived(message); return; }
		if (fGameOver) return;
		if (fShipX < fTargetX) fShipX += 6; if (fShipX > fTargetX) fShipX -= 6;
		if (fabs(fShipX - fTargetX) < 7) fShipX = fTargetX; if (fCooldown > 0) fCooldown--;
		if (fCooldown == 0 && !fBulletActive) Fire(); // auto-fire every 1.5 seconds
		if (fBulletActive) { fBulletY -= 13; if (fBulletY < 88) fBulletActive = false; }
		for (int32 i = 0; i < kPlanets; i++) {
			Planet& p = fPlanets[i];
			if (p.explosion > 0) { p.explosion--; if (p.explosion == 0) ResetPlanet(i, -p.size - 80); continue; }
			p.y += p.speed;
			if (fBulletActive && BulletHits(p)) { fBulletActive = false; p.explosion = kExplosion; fScore += 100; }
			if (p.y > Bounds().bottom) { fScore += 10; ResetPlanet(i, -p.size); }
			if (HitsShip(p)) { fLives--; p.explosion = kExplosion; if (fLives <= 0) fGameOver = true; }
		}
		if (!fPickup.active && rand() % 450 == 0) {
			fPickup.active = true; fPickup.x = 30 + rand() % 600; fPickup.y = 92;
		}
		if (fPickup.active) {
			fPickup.y += 2.1f;
			if (fPickup.y > Bounds().bottom - 40) fPickup.active = false;
			if (fabs((fPickup.x + 12) - (fShipX + 31)) < 42 && fPickup.y > Bounds().bottom - 145) {
				fScore += 50; if (fLives < 3) fLives++; fPickup.active = false;
			}
		}
		Invalidate();
	}

private:
	void DrawBackground(BRect b)
	{
		// Nebula clouds, a ringed giant and a distant moon create depth.
		SetHighColor(18, 20, 70); FillEllipse(BRect(32, 130, 470, 390)); SetHighColor(31, 34, 105); StrokeEllipse(BRect(32, 130, 470, 390));
		SetHighColor(43, 18, 87); FillEllipse(BRect(365, 170, 820, 540)); SetHighColor(91, 70, 202); StrokeEllipse(BRect(365, 170, 820, 540));
		SetHighColor(52, 80, 158); FillEllipse(BRect(555, 160, 750, 355)); SetHighColor(146, 221, 255); StrokeEllipse(BRect(535, 222, 774, 296));
		for (int32 i = 0; i < 125; i++) { float x = (i * 83 + 19) % (int32)b.Width(); float y = 90 + ((i * 47 + 7) % (int32)(b.Height() - 150)); int32 s = i % 13 == 0 ? 4 : (i % 4 == 0 ? 3 : 2); SetHighColor(i % 3 == 0 ? 139 : 205, i % 3 == 1 ? 226 : 244, 255); FillEllipse(BRect(x, y, x + s, y + s)); }
		SetHighColor(24, 96, 152);
		for (int32 x = 25; x < b.right; x += 55) StrokeLine(BPoint(x, b.bottom - 42), BPoint(b.Width() / 2, 300));
		for (int32 y = 330; y < b.bottom - 35; y += 28) StrokeLine(BPoint(10, y), BPoint(b.right - 10, y));
	}
	void DrawHud(BRect b)
	{
		SetHighColor(7, 22, 57); FillRect(BRect(10, 10, b.right - 10, 74)); SetHighColor(69, 235, 255); StrokeRect(BRect(10, 10, b.right - 10, 74));
		SetHighColor(71, 230, 255); SetFontSize(24); DrawString("NOVA STRIKER", BPoint(24, 38)); SetHighColor(171, 233, 255); SetFontSize(12); DrawString("A / D MOVE  |  CLICK TO STEER  |  AUTO LASER ONLINE", BPoint(25, 60));
		char info[90]; snprintf(info, sizeof(info), "SCORE: %ld    LIVES: %ld", (long)fScore, (long)fLives); SetHighColor(255, 227, 103); SetFontSize(14); DrawString(info, BPoint(b.right - 202, 38));
		SetHighColor(21, 49, 96); FillRect(BRect(b.right - 202, 47, b.right - 22, 65)); SetHighColor(fCooldown == 0 ? 94 : 255, fCooldown == 0 ? 255 : 132, 190);
		char weapon[50]; snprintf(weapon, sizeof(weapon), fCooldown == 0 ? "LASER: READY" : "CHARGING: %0.1fs", fCooldown / 33.0f); DrawString(weapon, BPoint(b.right - 194, 61));
	}
	void DrawPlanet(const Planet& p)
	{
		if (p.explosion > 0) { float r = p.size * (1.15f + (kExplosion - p.explosion) * .03f); SetHighColor(255, 98, 58); FillEllipse(BRect(p.x+p.size/2-r,p.y+p.size/2-r,p.x+p.size/2+r,p.y+p.size/2+r)); SetHighColor(255, 222, 91); FillEllipse(BRect(p.x+p.size*.22f,p.y+p.size*.22f,p.x+p.size*.78f,p.y+p.size*.78f)); SetHighColor(255,255,240); StrokeLine(BPoint(p.x-r,p.y-r),BPoint(p.x+p.size+r,p.y+p.size+r)); StrokeLine(BPoint(p.x+p.size+r,p.y-r),BPoint(p.x-r,p.y+p.size+r)); return; }
		float cx = p.x + p.size / 2;
		SetHighColor(43,24,92); FillTriangle(BPoint(cx,p.y),BPoint(p.x-8,p.y+p.size),BPoint(p.x+p.size+8,p.y+p.size));
		SetHighColor(217,74,171); FillTriangle(BPoint(cx,p.y+4),BPoint(p.x+3,p.y+p.size-4),BPoint(p.x+p.size-3,p.y+p.size-4));
		SetHighColor(101,229,255); FillTriangle(BPoint(cx,p.y+p.size*.22f),BPoint(cx-p.size*.18f,p.y+p.size*.64f),BPoint(cx+p.size*.18f,p.y+p.size*.64f));
		SetHighColor(255,228,95); FillEllipse(BRect(cx-p.size*.12f,p.y+p.size*.58f,cx+p.size*.12f,p.y+p.size*.82f));
		SetHighColor(249,186,255); StrokeLine(BPoint(p.x-8,p.y+p.size),BPoint(cx,p.y)); StrokeLine(BPoint(cx,p.y),BPoint(p.x+p.size+8,p.y+p.size));
	}
	void DrawShip(BRect b)
	{
		float y = b.bottom - 83; SetHighColor(255,105,53); FillTriangle(BPoint(fShipX+22,y+43),BPoint(fShipX+31,y+65),BPoint(fShipX+40,y+43)); SetHighColor(255,218,76); FillTriangle(BPoint(fShipX+25,y+42),BPoint(fShipX+31,y+56),BPoint(fShipX+37,y+42)); SetHighColor(35,126,218); FillTriangle(BPoint(fShipX+31,y-31),BPoint(fShipX-24,y+43),BPoint(fShipX+86,y+43)); SetHighColor(68,239,201); FillTriangle(BPoint(fShipX+31,y-24),BPoint(fShipX-4,y+37),BPoint(fShipX+66,y+37)); SetHighColor(22,67,152); FillTriangle(BPoint(fShipX-24,y+43),BPoint(fShipX-2,y+45),BPoint(fShipX+8,y+55)); FillTriangle(BPoint(fShipX+86,y+43),BPoint(fShipX+64,y+45),BPoint(fShipX+54,y+55)); SetHighColor(218,255,251); FillEllipse(BRect(fShipX+19,y-8,fShipX+43,y+17)); SetHighColor(12,76,164); FillEllipse(BRect(fShipX+23,y-5,fShipX+39,y+10)); SetHighColor(151,255,252); StrokeLine(BPoint(fShipX-24,y+43),BPoint(fShipX+86,y+43));
	}
	void DrawBullet() { if (!fBulletActive) return; SetHighColor(31,91,255); FillEllipse(BRect(fBulletX-7,fBulletY-11,fBulletX+7,fBulletY+11)); SetHighColor(224,255,255); FillEllipse(BRect(fBulletX-3,fBulletY-7,fBulletX+3,fBulletY+7)); }
	void DrawPickup()
	{
		if (!fPickup.active) return;
		SetHighColor(255, 178, 48); FillEllipse(BRect(fPickup.x - 4, fPickup.y - 4, fPickup.x + 28, fPickup.y + 28));
		SetHighColor(255, 241, 135); FillEllipse(BRect(fPickup.x, fPickup.y, fPickup.x + 24, fPickup.y + 24));
		SetHighColor(30, 101, 202); FillRect(BRect(fPickup.x + 9, fPickup.y + 4, fPickup.x + 15, fPickup.y + 20));
		FillRect(BRect(fPickup.x + 4, fPickup.y + 9, fPickup.x + 20, fPickup.y + 15));
	}
	void DrawGameOverBackground(BRect b)
	{
		// A red emergency sky replaces the generic dark overlay.
		SetHighColor(38, 5, 22); FillRect(BRect(0, 76, b.right, b.bottom));
		SetHighColor(112, 18, 42); FillEllipse(BRect(-100, 115, 350, 565));
		SetHighColor(255, 84, 64); StrokeEllipse(BRect(-100, 115, 350, 565));
		SetHighColor(80, 9, 36); FillEllipse(BRect(395, 125, 875, 600));
		SetHighColor(255, 134, 53);
		for (int32 i = 0; i < 16; i++) {
			float x = (i * 97 + 31) % (int32)b.Width(); float y = 115 + (i * 61) % (int32)(b.Height() - 160);
			FillEllipse(BRect(x, y, x + 5, y + 5));
		}
		SetHighColor(0, 0, 0, 140); FillRect(BRect(0, 94, b.right, b.bottom - 48));
	}
	void DrawFooter(BRect b) { SetHighColor(7,22,57); FillRect(BRect(10,b.bottom-30,b.right-10,b.bottom-10)); SetHighColor(65,220,255); StrokeRect(BRect(10,b.bottom-30,b.right-10,b.bottom-10)); SetHighColor(160,235,255); SetFontSize(11); DrawString("WAVE 01: INTERCEPTOR SQUADRON  //  DESTROY ENEMIES +100",BPoint(20,b.bottom-15)); }
	void Fire() { if (fCooldown > 0 || fBulletActive || fGameOver) return; fBulletX=fShipX+31; fBulletY=Bounds().bottom-120; fBulletActive=true; fCooldown=kCooldown; }
	bool BulletHits(const Planet& p) const { float dx=fBulletX-(p.x+p.size/2), dy=fBulletY-(p.y+p.size/2); return dx*dx+dy*dy < (p.size/2+8)*(p.size/2+8); }
	bool HitsShip(const Planet& p) const { float y=Bounds().bottom-83; return p.x+p.size>fShipX && p.x<fShipX+62 && p.y+p.size>y && p.y<y+55; }
	void ResetPlanet(int32 i,float y) { fPlanets[i].size=32+rand()%35; fPlanets[i].x=22+rand()%610; fPlanets[i].y=y; fPlanets[i].speed=1.0f+(rand()%18)/10.0f; fPlanets[i].explosion=0; }
	void Restart() { fShipX=fTargetX=320; fScore=0; fLives=3; fCooldown=0; fBulletActive=false; fPickup.active=false; fGameOver=false; for(int32 i=0;i<kPlanets;i++) ResetPlanet(i,-80.0f*i); }
	Planet fPlanets[kPlanets]; Pickup fPickup; float fShipX,fTargetX,fBulletX,fBulletY; int32 fScore,fLives,fCooldown; bool fBulletActive,fGameOver;
};

class GameWindow : public BWindow { public: GameWindow() : BWindow(BRect(120,100,820,620),"Haiku Sky Defender",B_TITLED_WINDOW,B_QUIT_ON_WINDOW_CLOSE) { fGame=new GameView(); AddChild(fGame); fRunner=new BMessageRunner(BMessenger(fGame),new BMessage(MSG_FRAME),30000); } ~GameWindow(){delete fRunner;} private: GameView* fGame; BMessageRunner* fRunner; };
class GameApp : public BApplication { public: GameApp():BApplication("application/x-vnd.btl.skydefender"){} void ReadyToRun() override { (new GameWindow())->Show(); } };
int main(){ GameApp app; app.Run(); return 0; }
