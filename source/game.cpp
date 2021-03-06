//==================================================================================================================
//
// Gameの描画 [game.cpp]
// Author : Seiya Takahashi
//
//==================================================================================================================

//==================================================================================================================
//	インクルードファイル
//==================================================================================================================
#include "game.h"
#include "number.h"
#include "fade.h"
#include "inputKeyboard.h"
#include "meshField.h"
#include "scene2D.h"
#include "scene3D.h"
#include "camera.h"
#include "light.h"
#include "time.h"
#include "renderer.h"
#include "pause.h"
#include "inputGamepad.h"
#include "meshSphere.h"
#include "player.h"
#include "modelCharacter.h"
#include "stone.h"
#include "3DBoxCollider.h"
#include "hitpoint.h"
#include "bar.h"
#include "wall.h"
#include "debugProc.h"
#include "objManager.h"
#include "CylinderCollider.h"
#include "CapsuleCollider.h"
#include "PolygonCollider.h"
#include "UI_KO.h"
#include "UI_gameStart.h"
#include "UI_gameResult.h"
#include "UI_game.h"
#include "3DEffect.h"
#include "CharEffectOffset.h"
#include "3DParticle.h"
#include "transformBar.h"
#include "polyCollMana.h"
#include "shadow.h"
#include "sound.h"

//==================================================================================================================
//	マクロ定義
//==================================================================================================================
#define MAX_ROUND			(3)										// 最大ラウンド数
#define PLAYER_START_POS_X 390.0f									// プレイヤーの初期位置X
#define PLAYER_START_POS_Z -585.0f									// プレイヤーの初期位置Z
#define RESPAWN_SIZE 0.000001f										// リスポーンモデルの大きさ

#define TIME_CREATE_STONE	(5 * ONE_SECOND_FPS)					// ストーンを生成する時間
#define TIME_KO_AFTER		(60)									// KOの後の時間

#define TIME_FADE_NEXTROUND	(10)									// ラウンド切り替えのフェード時間

#define DEFAULTPOS_1P		(D3DXVECTOR3(-160.0f, 90.0f, 100.0f))	// 1Pプレイヤーの初期座標
#define DEFAULTPOS_2P		(D3DXVECTOR3(10.0f, 60.0f, -160.0f))		// 2Pプレイヤーの初期座標

//==================================================================================================================
//	静的メンバ変数宣言
//==================================================================================================================
CPlayer				*CGame::m_pPlayer[MAX_PLAYER]	= {};							// キャラクター情報
CMeshField			*CGame::m_pMeshField			= NULL;							// メッシュフィールド情報
CCamera				*CGame::m_pCamera				= NULL;							// カメラ情報
CLight				*CGame::m_pLight				= NULL;							// ライト情報
CLogo				*CGame::m_pLogo					= NULL;							// ロゴ情報
CPause				*CGame::m_pPause				= NULL;							// ポーズ情報
CMeshSphere			*CGame::m_pMeshSphere			= NULL;							// メッシュ球の情報
CTime				*CGame::m_pTime					= NULL;							// タイム情報
CWall				*CGame::m_pWall					= NULL;							// 壁のポインタ
CUI_game			*CGame::m_pUI					= NULL;							// UIポインタ
CUIKO				*CGame::m_pUIKO					= nullptr;						// KOのポインタ
CUI_GameStart		*CGame::m_pUIGameStart			= nullptr;						// ゲーム開始時のUIのポインタ
CUI_GameResult		*CGame::m_pUIGameResult			= nullptr;						// ゲームリザルトのUIのポインタ
CGame::GAMESTATE	CGame::m_gameState				= CGame::GAMESTATE_NONE;		// ゲーム状態
int					CGame::m_nCounterGameState		= NULL;							// ゲームの状態管理カウンター
int					CGame::m_nNumStone				= 0;							// 生成したストーンの数
int					CGame::m_nCntDecide				= 0;							// ストーン生成のタイミングを決めるカウンタ
int					CGame::m_nRound					= 0;							// ラウンド数
int					CGame::m_nRoundAll				= 0;							// 全ラウンド数
NUM_PLAYER			CGame::m_winPlayer				= NUM_PLAYER::PLAYER_NONE;		// 勝利したプレイヤー
NUM_PLAYER			CGame::m_losePlayer				= NUM_PLAYER::PLAYER_NONE;		// 負けたプレイヤー
CObjectManager		*CGame::m_pObjMana				= nullptr;						// オブジェクトマネージャーのポインタ
CTransformBar		*CGame::m_pTransformBar			= nullptr;						// 変身バーポインタ
bool				CGame::m_bSetPos[STONE_POS]		= {};							// ストーンの生成場所に生成されているか
bool				CGame::m_bGetType[CStone::STONE_ID_MAX] = {};					// ストーンの生成場所に生成されているか
int					CGame::m_nStageType				= STAGE_1;							// ステージのタイプ

int					CGame::m_nPlayerType[MAX_PLAYER] = {};	// キャラクターセレクト時のタイプを保存

CPolyCollMana* CGame::m_pPolyCollMana = {};							// ポリゴンコライダーのポインタ

//==================================================================================================================
//	コンストラクタ
//==================================================================================================================
CGame::CGame()
{

}

//==================================================================================================================
//	デストラクタ
//==================================================================================================================
CGame::~CGame()
{

}

//==================================================================================================================
//	初期化処理
//==================================================================================================================
void CGame::Init(void)
{
	/* ロード */
	CNumber::Load();						// 数字テクスチャロード
	CMeshField::Load();						// 床テクスチャロード
	CPause::Load();							// ポーズテクスチャロード
	CMeshSphere::Load();					// メッシュ球のテクスチャロード
	CStone::Load();							// ストーンの読み込み
	CBar::Load();							// Barテクスチャロード
	C3DBoxCollider::Load();					// 3Dボックスコライダーの読み込み
	CWall::Load();							// 壁のロード
	CCapsuleCollider::Load();				// カプセルコライダーのロード
	CUIKO::Load();							// KOのロード
	CUI_GameStart::Load();					// 開始UIのロード
	CUI_GameResult::Load();
	CUI_game::Load();						// UIロード
	CPolygonCollider::Load();
	CShadow::Load();						// 影テクスチャロード

	// 3Dエフェクトの作成
	C3DEffect *p3DEffect;
	// 生成
	p3DEffect = new C3DEffect;
	// 作成
	p3DEffect->Make();

	/* 生成 */
	C3DBoxCollider::Create();										// ボックスコライダーの生成
	m_pPlayer[PLAYER_ONE] = CPlayer::Create(PLAYER_ONE, (CHARACTER_TYPE)m_nPlayerType[PLAYER_ONE]);	// プレイヤー生成
	m_pPlayer[PLAYER_TWO] = CPlayer::Create(PLAYER_TWO, (CHARACTER_TYPE)m_nPlayerType[PLAYER_TWO]);	// プレイヤー生成
	m_pPlayer[PLAYER_ONE]->SetPos(DEFAULTPOS_1P);
	m_pPlayer[PLAYER_TWO]->SetPos(DEFAULTPOS_2P);

	m_pObjMana    = CObjectManager::Create((STAGETYPE)m_nStageType);// オブジェクトマネージャーの生成
	m_pWall       = CWall::Create(CWall::WALLTEX_FIELD);			// 壁の生成
	m_pCamera     = CCamera::Create();								// カメラの生成処理
	m_pLight      = CLight::Create();								// ライトの生成処理
	m_pMeshSphere = CMeshSphere::Create();							// メッシュ球の生成処理

	m_pMeshField  = CMeshField::Create(INTEGER2(4, 4), D3DXVECTOR3(600.0f, 0.0f, 600.0f), D3DXVECTOR3(0.0f, -40.0f, 50.0f));// メッシュフィールド生成
	m_pUI         = CUI_game::Create();								// UIの生成処理
	m_pTransformBar = CTransformBar::Create(TIME_TRANS);			// 変身バー生成処理
	m_pTime       = CTime::Create();								// タイム生成
	m_pPause      = CPause::Create();								// ポーズの生成処理

	// ポリゴンコライダーの生成
	m_pPolyCollMana = CPolyCollMana::Create();

	/* ゲームの初期化 */
	m_gameState = GAMESTATE_BEFORE;
	m_nCounterGameState = 0;				// ゲームの状態管理カウンターを0にする
	m_nNumStone			= 0;				// 値を初期化
	m_nCntDecide		= 0;				// 値を初期化
	m_nCntAny			= 0;
	m_nRound			= 0;
	m_nRoundAll			= MAX_ROUND;
	m_roundPoint		= INTEGER2(0, 0);

	for (int nCnt = 0; nCnt < STONE_POS; nCnt++)
	{
		m_bSetPos[nCnt] = false;
	}

	for (int nCnt = 0; nCnt < CStone::STONE_ID_MAX; nCnt++)
	{
		m_bGetType[nCnt] = false;
	}
}

//==================================================================================================================
//	終了処理
//==================================================================================================================
void CGame::Uninit(void)
{
	// ゲーム状態を通常にする
	m_gameState = GAMESTATE_NORMAL;

	// 全ての開放
	CScene2D::ReleaseAll();				// 2Dのもの全て破棄
	CScene3D::ReleaseAll();				// 3Dのもの全て破棄

	/* アンロード */
	CCapsuleCollider::Unload();
	CWall::Unload();					// 壁の開放
	C3DBoxCollider::Unload();			// 3Dボックスコライダーの開放
	CStone::Unload();					// ストーンの開放
	CNumber::Unload();					// 数字テクスチャアンロード
	CMeshField::Unload();				// 床テクスチャアンロード
	CBar::Unload();						// Barテクスチャアンロード
	CUIKO::Unload();					// KOのアンロード
	CUI_GameStart::Unload();			// 開始時のUIのアンロード
	CUI_GameResult::Unload();
	CUI_game::Unload();					// UIアンロード
	CPolygonCollider::Unload();
	CShadow::Unload();					// 影テクスチャアンロード

	// 万が一残っていた場合
	if (m_pUIGameStart)
	{
		// 破棄
		m_pUIGameStart->Uninit();
		delete m_pUIGameStart;
		m_pUIGameStart = nullptr;
	}

	// 万が一残っていた場合
	if (m_pUIGameResult)
	{
		// 破棄
		m_pUIGameResult->Uninit();
		delete m_pUIGameResult;
		m_pUIGameResult = nullptr;
	}

	// 万が一残っていた場合
	if (m_pUIKO)
	{
		// 破棄
		m_pUIKO->Uninit();
		delete m_pUIKO;
		m_pUIKO = nullptr;
	}

	if (m_pUI)
	{
		// 破棄
		m_pUI->Uninit();
		delete m_pUI;
		m_pUI = nullptr;
	}

	// ポーズの終了処理
	m_pPause->Uninit();

	m_pObjMana = nullptr;				// ポインタNULL

	delete m_pPause;					// メモリ削除
	m_pPause = nullptr;					// ポインタNULL

	delete m_pCamera;					// メモリ削除
	m_pCamera = nullptr;				// ポインタNULL

	delete m_pLight;					// メモリ削除
	m_pLight = nullptr;					// ポインタNULL

	if (m_pTime)
	{
		m_pTime->Uninit();
		delete m_pTime;
		m_pTime = nullptr;					// ポインタNULL
	}

	if (m_pTransformBar)
	{
		m_pTransformBar->Uninit();
		delete m_pTransformBar;
		m_pTransformBar = nullptr;
	}
}

//==================================================================================================================
//	更新処理
//==================================================================================================================
void CGame::Update(void)
{
	// 開始前
	if (m_gameState == GAMESTATE_BEFORE)
		GameBefore();
	// 通常のとき
	if (m_gameState == GAMESTATE_NORMAL)
		GameNormal();
	// ゲーム状態がポーズのとき
	else if (m_gameState == GAMESTATE_PAUSE)
		GamePause();
	// KOのとき
	else if (m_gameState == GAMESTATE_KO)
		GameKO();
	// KOの後
	else if (m_gameState == GAMESTATE_KO_AFTER)
		GameKOAfter();
	// 次のラウンド
	else if (m_gameState == GAMESTATE_NEXTROUND)
		NextRound();
	// リザルト
	else if (m_gameState == GAMESTATE_RESULT)
		GameResult();

	if (m_pUI)
		m_pUI->Update();
	if (m_pTime)
		m_pTime->Update();
	if (m_pTransformBar)
		m_pTransformBar->Update();

#ifdef _DEBUG
	// キーボードの[0]を押したとき
	if (m_gameState != GAMESTATE_RESULT && CManager::GetInputKeyboard()->GetKeyboardTrigger(DIK_RETURN) && CManager::GetInputKeyboard()->GetKeyboardPress(DIK_LSHIFT))
	{
		// フェード取得
		CFade::FADE fade = CFade::GetFade();

		// フェードが何もない時
		if (fade == CFade::FADE_NONE)
			// フェードを設定する
			CFade::SetFade(CRenderer::MODE_TITLE, DEFAULT_FADE_TIME);
	}

	if (CManager::GetInputKeyboard()->GetKeyboardPress(DIK_LSHIFT))
	{
		C3DParticle::Set(&m_pPlayer[0]->GetPos(), &m_pPlayer[0]->GetRot(), C3DParticle::CHARGE_R);
		C3DParticle::Set(&m_pPlayer[0]->GetPos(), &m_pPlayer[0]->GetRot(), C3DParticle::CHARGE_G);
		C3DParticle::Set(&m_pPlayer[0]->GetPos(), &m_pPlayer[0]->GetRot(), C3DParticle::CHARGE_B);
	}
#endif // _DEBUG
}

//==================================================================================================================
//	描画処理
//==================================================================================================================
void CGame::Draw(void)
{
	// カメラの設定
	m_pCamera->SetCamera();

	// カメラの描画処理
	m_pCamera->Draw();

	if (m_pUI)
		m_pUI->Draw();
	if (m_pTime)
		m_pTime->Draw();
	if (m_pTransformBar)
		m_pTransformBar->Draw();

	// ポーズ状態がtrueのとき
	if (m_pPause->GetPause() == true)
		// ポーズの更新処理
		m_pPause->Draw();

	// ゲーム開始時のUIの描画
	if (m_pUIGameStart)
		m_pUIGameStart->Draw();

	// KOの描画
	if (m_pUIKO)
		m_pUIKO->Draw();

	// リザルトのUIの描画
	if (m_pUIGameResult)
		m_pUIGameResult->Draw();
}

//==================================================================================================================
//	生成処理
//==================================================================================================================
CGame * CGame::Create(void)
{
	// メモリ確保
	CGame *pGame = new CGame;
	// 初期化
	pGame->Init();
	// 値を返す
	return pGame;
}

//==================================================================================================================
//	次のモードへ移行
//==================================================================================================================
void CGame::SetNextMode(const int nextMode)
{
	// nullcheck
	if (m_pUIGameResult)
	{
		// 破棄
		m_pUIGameResult->Uninit();
		delete m_pUIGameResult;
		m_pUIGameResult = nullptr;
	}

	m_gameState = GAMESTATE_END;

	// フェード取得
	CFade::FADE fade = CFade::GetFade();

	// フェードが何もない時
	if (fade == CFade::FADE_NONE)
		// フェードを設定する
		CFade::SetFade((CRenderer::MODE)nextMode, DEFAULT_FADE_TIME);
}

//==================================================================================================================
//	どこからでも呼び出せるストーン生成処理
//==================================================================================================================
void CGame::AppearStone(void)
{
	// ランダムでポイントを決める
	int RandPos = CKananLibrary::DecideRandomValue(STONE_POS, m_bSetPos, false);
	// ランダムでポイントを決める
	int RandType = CKananLibrary::DecideRandomValue(CStone::STONE_ID_MAX, m_bGetType, false);
	// 決められた位置からランダムで生成
	CStone::Create(RandPos, (CStone::STONE_ID)RandType, CObjectManager::GetDefaultStonePos(m_nStageType, RandPos));
	// 生成された
	m_bSetPos[RandPos] = true;
	// 生成された
	m_bGetType[RandType] = true;
	// 出現数を加算
	m_nNumStone++;
}

//==================================================================================================================
//	ゲームの前の更新
//==================================================================================================================
void CGame::GameBefore(void)
{
	if (!m_pUIGameStart)
		m_pUIGameStart = CUI_GameStart::Create();

	// カメラの更新処理
	m_pCamera->Update();

	// ライトの更新処理
	m_pLight->Update();

	if (m_pUIGameStart)
		m_pUIGameStart->Update();
}

//==================================================================================================================
//	通常の更新
//==================================================================================================================
void CGame::GameNormal(void)
{
	// nullcheck
	if (m_pUIGameStart)
	{
		// 破棄
		m_pUIGameStart->Uninit();
		delete m_pUIGameStart;
		m_pUIGameStart = nullptr;
	}

	// カメラの更新処理
	m_pCamera->Update();

	// ライトの更新処理
	m_pLight->Update();

	// ストーンを生成するか決める
	DecideCreateStone();

	// どちらかのプレイヤーのライフが0
	if (GetPlayer(PLAYER_ONE)->GetLife() <= 0 ||
		GetPlayer(PLAYER_TWO)->GetLife() <= 0)
	{
		// KO
		m_gameState = GAMESTATE_KO;
		// 効果音の再生
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_KNOCKOUT);
	}

	// ポーズの切り替え
	if (CManager::GetInputKeyboard()->GetKeyboardTrigger(DIK_P))
		SwitchPause();
}

//==================================================================================================================
//	ポーズの更新
//==================================================================================================================
void CGame::GamePause(void)
{
	// ポーズの更新処理
	m_pPause->Update();

	// リトライ
	if (m_gameState == GAMESTATE_START_OVER)
		CFade::SetFade(CRenderer::MODE_GAME, DEFAULT_FADE_TIME);
	// タイトルに戻る
	else if (m_gameState == GAMESTATE_BREAK)
		CFade::SetFade(CRenderer::MODE_TITLE, DEFAULT_FADE_TIME);
}

//==================================================================================================================
//	KOの更新
//==================================================================================================================
void CGame::GameKO(void)
{
	// KOのUIを生成
	if (!m_pUIKO)
		m_pUIKO = CUIKO::Create();

	// KOの更新
	if (m_pUIKO)
		m_pUIKO->Update();
}

//==================================================================================================================
//	KOの後の更新
//==================================================================================================================
void CGame::GameKOAfter(void)
{
	// カウンタの加算
	m_nCntAny++;
	// 一定時間で次へ
	if (m_nCntAny >= TIME_KO_AFTER &&
		m_pPlayer[PLAYER_ONE]->GetStandState() != CCharacter::STANDSTATE_JUMP && 
		m_pPlayer[PLAYER_TWO]->GetStandState() != CCharacter::STANDSTATE_JUMP)
	{
		m_nCntAny = 0;
		m_gameState = GAMESTATE_NEXTROUND;
		return;
	}

	// カメラの更新処理
	m_pCamera->Update();
	// ライトの更新処理
	m_pLight->Update();
}

//==================================================================================================================
//	ポーズの切り替え
//==================================================================================================================
void CGame::SwitchPause(void)
{
	// ゲーム状態がポーズのとき
	if (m_gameState == GAMESTATE_PAUSE)
	{
		// ゲーム状態をNORMALにする
		m_gameState = GAMESTATE_NORMAL;
		// ポーズ状態の設定
		m_pPause->SetPause(false);
	}
	else
	{// ゲーム状態がポーズじゃないとき
	 // ゲーム状態をポーズにする
		m_gameState = GAMESTATE_PAUSE;
		// ポーズ状態の設定
		m_pPause->SetPause(true);
	}
}

//==================================================================================================================
//	次のラウンドへ
//==================================================================================================================
void CGame::NextRound(void)
{
	// nullcheck
	if (m_pUIKO)
	{
		// 破棄
		m_pUIKO->Uninit();
		delete m_pUIKO;
		m_pUIKO = nullptr;
	}

	// ラウンド数を加算
	m_nRound++;
	// プレイヤーのラウンドポイントを加算
	if (GetPlayer(PLAYER_ONE)->GetLife() <= 0 &&
		GetPlayer(PLAYER_TWO)->GetLife() <= 0)
		m_nRound--;
	else if (GetPlayer(PLAYER_ONE)->GetLife() > 0)
		m_roundPoint.nX++;
	else if (GetPlayer(PLAYER_TWO)->GetLife() > 0)
		m_roundPoint.nY++;

	// どちらかが最大まで得点したら終了
	if (m_roundPoint.nX == (int)((float)m_nRoundAll / 2.0f + 0.5f) ||
		m_roundPoint.nY == (int)((float)m_nRoundAll / 2.0f + 0.5f))
	{
		// 勝者を決める
		if (m_roundPoint.nX > m_roundPoint.nY)
		{
			m_winPlayer = PLAYER_ONE;
			m_losePlayer = PLAYER_TWO;
		}
		else
		{
			m_winPlayer = PLAYER_TWO;
			m_losePlayer = PLAYER_ONE;
		}		// リザルトへ
		m_gameState = GAMESTATE_RESULT;
	}
	// それ以外は再バトル
	else
	{
		// フェード取得
		CFade *pFade = CManager::GetRenderer()->GetFade();
		// フェード時間を設定し、空のフェード実行
		pFade->SetFade(CRenderer::MODE_NONE, TIME_FADE_NEXTROUND);
		// バトル前へ
		m_gameState = GAMESTATE_BEFORE;
		m_pPlayer[PLAYER_ONE]->SetPos(DEFAULTPOS_1P);
		m_pPlayer[PLAYER_TWO]->SetPos(DEFAULTPOS_1P);
		m_pPlayer[PLAYER_ONE]->SetLife(m_pPlayer[PLAYER_ONE]->GetMaxLife());
		m_pPlayer[PLAYER_TWO]->SetLife(m_pPlayer[PLAYER_TWO]->GetMaxLife());
	}
}

//==================================================================================================================
//	ゲームのリザルト
//==================================================================================================================
void CGame::GameResult(void)
{
	// 生成
	if (!m_pUIGameResult)
		m_pUIGameResult = CUI_GameResult::Create();

	// 勝利時の行動
	m_pPlayer[m_winPlayer]->VictoryAction();

	// 更新
	if (m_pUIGameResult)
		m_pUIGameResult->Update();
}

//==================================================================================================================
//	ストーンを生成するか決める
//==================================================================================================================
void CGame::DecideCreateStone(void)
{
	// カウンタを加算
	m_nCntDecide++;

#ifdef _DEBUG
	// 所持数・出現数の合計が3未満
	if (CManager::GetInputKeyboard()->GetKeyboardTrigger(DIK_1) && CManager::GetInputKeyboard()->GetKeyboardPress(DIK_LSHIFT) && 
		m_nNumStone + GetPlayer(PLAYER_ONE)->GetNumStone() + GetPlayer(PLAYER_TWO)->GetNumStone() < 3)
		// ストーン生成
		AppearStone();
#endif

	// 時間以内
	if (m_nCntDecide <= TIME_CREATE_STONE)
		// 処理を終える
		return;

	// 所持数・出現数の合計が3未満
	if (m_nNumStone + GetPlayer(PLAYER_ONE)->GetNumStone() + GetPlayer(PLAYER_TWO)->GetNumStone() < 3)
		// ストーン生成
		AppearStone();

	// カウンタを初期化
	m_nCntDecide = 0;
}
