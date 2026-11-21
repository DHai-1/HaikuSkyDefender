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
const int32 kMeteorCount = 6;

using namespace std;

struct Meteor {
	float x;
	float y;
	float speed;
	float size;
};

class GameView : public BView {
public:
	GameView()
		: BView(BRect(0, 0, 700, 520), "game", B_FOLLOW_ALL,
			B_WILL_DRAW | B_NAVIGABLE),
		  fShipX(320), fScore(0), fLives(3), fGameOver(false)
	{
		SetViewColor(8, 16, 38);
		srand((unsigned)time(NULL));
		for (int32 i = 0; i < kMeteorCount; i++)
			ResetMeteor(i, -30.0f * i);
	}

	void AttachedToWindow() override
	{
		MakeFocus(true);
	}

	void Draw(BRect) override
	{
		BRect bounds = Bounds();
		SetHighColor(4, 9, 28);
		FillRect(bounds);

		// Decorative nebula bands and layered star field.
		SetHighColor(16, 31, 80);
		FillEllipse(BRect(80, 100, 510, 340));
		SetHighColor(20, 54, 92);
		FillEllipse(BRect(420, 185, 850, 480));
		for (int32 i = 0; i < 105; i++) {
			float x = (i * 83 + 17) % (int32)bounds.Width();
			float y = 80 + ((i * 47 + 13) % (int32)(bounds.Height() - 130));
			int32 size = i % 11 == 0 ? 4 : (i % 4 == 0 ? 3 : 2);
			SetHighColor(i % 3 == 0 ? 170 : 210, i % 3 == 1 ? 210 : 240, 255);
			FillEllipse(BRect(x, y, x + size, y + size));
		}

		// HUD panel.
		SetHighColor(8, 22, 58);
		FillRect(BRect(10, 10, bounds.right - 10, 73));
		SetHighColor(65, 220, 255);
		StrokeRect(BRect(10, 10, bounds.right - 10, 73));
		SetHighColor(80, 220, 255);
		SetFontSize(23);
		DrawString("HAIKU SKY DEFENDER", BPoint(24, 37));
		SetFontSize(13);
		SetHighColor(155, 230, 255);
		DrawString("ARCADE MISSION 01  |  A / D: MOVE  |  R: RESTART", BPoint(25, 59));

		char info[80];
		snprintf(info, sizeof(info), "SCORE: %ld     LIVES: %ld",
			(long)fScore, (long)fLives);
		SetHighColor(255, 230, 110);
		DrawString(info, BPoint(bounds.right - 210, 37));

		for (int32 i = 0; i < kMeteorCount; i++) {
			float x = fMeteors[i].x;
			float y = fMeteors[i].y;
			float s = fMeteors[i].size;
			SetHighColor(108, 75, 82);
			FillEllipse(BRect(x - 3, y - 3, x + s + 3, y + s + 3));
			SetHighColor(236, 113, 69);
			FillEllipse(BRect(x, y, x + s, y + s));
			SetHighColor(255, 191, 100);
			StrokeEllipse(BRect(x, y, x + s, y + s));
			SetHighColor(171, 69, 59);
			FillEllipse(BRect(x + s * .22f, y + s * .25f, x + s * .48f, y + s * .50f));
			FillEllipse(BRect(x + s * .58f, y + s * .55f, x + s * .78f, y + s * .75f));
		}

		// Player ship, cockpit, wings and animated engine flame.
		float shipY = bounds.bottom - 62;
		SetHighColor(255, 140, 55);
		FillTriangle(BPoint(fShipX + 16, shipY + 27), BPoint(fShipX + 25, shipY + 48),
			BPoint(fShipX + 34, shipY + 27));
		SetHighColor(44, 186, 255);
		FillTriangle(BPoint(fShipX + 25, shipY - 20), BPoint(fShipX - 12, shipY + 28),
			BPoint(fShipX + 62, shipY + 28));
		SetHighColor(53, 241, 199);
		FillTriangle(BPoint(fShipX + 25, shipY - 12), BPoint(fShipX + 1, shipY + 24),
			BPoint(fShipX + 49, shipY + 24));
		SetHighColor(212, 255, 250);
		FillEllipse(BRect(fShipX + 16, shipY - 4, fShipX + 34, shipY + 14));
		SetHighColor(20, 76, 146);
		FillEllipse(BRect(fShipX + 19, shipY - 2, fShipX + 31, shipY + 9));
		SetHighColor(125, 245, 255);
		StrokeLine(BPoint(fShipX - 12, shipY + 28), BPoint(fShipX + 62, shipY + 28));

		if (fGameOver) {
			SetHighColor(0, 0, 0, 190);
			FillRect(BRect(0, 90, bounds.right, bounds.bottom - 80));
			SetHighColor(255, 110, 110);
			SetFontSize(32);
			DrawString("GAME OVER", BPoint(bounds.Width() / 2 - 110, 210));
			SetHighColor(255, 255, 255);
			SetFontSize(17);
			DrawString("Press R to launch a new mission", BPoint(bounds.Width() / 2 - 130, 246));
		}
	}

	void KeyDown(const char* bytes, int32) override
	{
		if (bytes[0] == 'a' || bytes[0] == 'A')
			fShipX = fShipX > 14 ? fShipX - 22 : 0;
		else if (bytes[0] == 'd' || bytes[0] == 'D')
			fShipX = fShipX < Bounds().right - 70 ? fShipX + 22 : Bounds().right - 48;
		else if (bytes[0] == 'r' || bytes[0] == 'R')
			Restart();
		Invalidate();
	}

	void MessageReceived(BMessage* message) override
	{
		if (message->what != MSG_FRAME) {
			BView::MessageReceived(message);
			return;
		}
		if (fGameOver)
			return;

		for (int32 i = 0; i < kMeteorCount; i++) {
			fMeteors[i].y += fMeteors[i].speed;
			if (fMeteors[i].y > Bounds().bottom) {
				fScore += 10;
				ResetMeteor(i, -fMeteors[i].size);
			}
			if (HitsShip(fMeteors[i])) {
				fLives--;
				ResetMeteor(i, -fMeteors[i].size);
				if (fLives == 0)
					fGameOver = true;
			}
		}
		Invalidate();
	}

private:
	void ResetMeteor(int32 index, float y)
	{
		fMeteors[index].size = 18 + rand() % 25;
		fMeteors[index].x = rand() % 620;
		fMeteors[index].y = y;
		fMeteors[index].speed = 2.0f + (rand() % 30) / 10.0f;
	}

	bool HitsShip(const Meteor& meteor)
	{
		float shipY = Bounds().bottom - 62;
		return meteor.x + meteor.size > fShipX && meteor.x < fShipX + 48
			&& meteor.y + meteor.size > shipY && meteor.y < shipY + 25;
	}

	void Restart()
	{
		fShipX = 320;
		fScore = 0;
		fLives = 3;
		fGameOver = false;
		for (int32 i = 0; i < kMeteorCount; i++)
			ResetMeteor(i, -30.0f * i);
	}

	Meteor fMeteors[kMeteorCount];
	float fShipX;
	int32 fScore;
	int32 fLives;
	bool fGameOver;
};

class GameWindow : public BWindow {
public:
	GameWindow()
		: BWindow(BRect(120, 100, 820, 620), "Haiku Sky Defender",
			B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		fGame = new GameView();
		AddChild(fGame);
		fRunner = new BMessageRunner(BMessenger(fGame), new BMessage(MSG_FRAME), 30000);
	}

	~GameWindow() { delete fRunner; }

private:
	GameView* fGame;
	BMessageRunner* fRunner;
};

class GameApp : public BApplication {
public:
	GameApp() : BApplication("application/x-vnd.btl-skydefender") {}
	void ReadyToRun() override { (new GameWindow())->Show(); }
};

int main()
{
	GameApp app;
	app.Run();
	return 0;
}
