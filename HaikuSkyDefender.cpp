#include <Application.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>
#include <Window.h>

#include <math.h>
#include <cstdio>
#include <stdlib.h>
#include <time.h>

using namespace std;

enum { MSG_FRAME = 'fram' };
const int32 kMeteorCount = 6;

struct Meteor {
	float x;
	float y;
	float speed;
	float size;
};

class GameView : public BView {
public:
	GameView()
		: BView("game", B_WILL_DRAW | B_NAVIGABLE),
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
		SetHighColor(8, 16, 38);
		FillRect(bounds);

		// Star field
		SetHighColor(170, 205, 255);
		for (int32 i = 0; i < 70; i++) {
			float x = (i * 83) % (int32)bounds.Width();
			float y = (i * 47) % (int32)(bounds.Height() - 70);
			FillEllipse(BRect(x, y, x + 2, y + 2));
		}

		SetHighColor(80, 220, 255);
		SetFontSize(23);
		DrawString("HAIKU SKY DEFENDER", BPoint(22, 34));
		SetFontSize(13);
		DrawString("A / D: move     R: restart", BPoint(24, 57));

		char info[80];
		snprintf(info, sizeof(info), "SCORE: %ld     LIVES: %ld", (long)fScore, (long)fLives);
		SetHighColor(255, 230, 110);
		DrawString(info, BPoint(bounds.right - 220, 34));

		for (int32 i = 0; i < kMeteorCount; i++) {
			SetHighColor(255, 118, 65);
			FillEllipse(BRect(fMeteors[i].x, fMeteors[i].y,
				fMeteors[i].x + fMeteors[i].size, fMeteors[i].y + fMeteors[i].size));
			SetHighColor(255, 190, 105);
			StrokeEllipse(BRect(fMeteors[i].x, fMeteors[i].y,
				fMeteors[i].x + fMeteors[i].size, fMeteors[i].y + fMeteors[i].size));
		}

		float shipY = bounds.bottom - 52;
		SetHighColor(70, 240, 190);
		FillRect(BRect(fShipX, shipY, fShipX + 48, shipY + 18));
		SetHighColor(210, 255, 245);
		FillRect(BRect(fShipX + 17, shipY - 10, fShipX + 31, shipY));
		SetHighColor(70, 150, 255);
		StrokeLine(BPoint(fShipX, shipY + 18), BPoint(fShipX - 12, shipY + 29));
		StrokeLine(BPoint(fShipX + 48, shipY + 18), BPoint(fShipX + 60, shipY + 29));

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
		float shipY = Bounds().bottom - 52;
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
