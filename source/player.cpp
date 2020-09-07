//==================================================================================================================
//
// プレイヤー処理[player.cpp]
// Author : Seiya Takahashi
//
//==================================================================================================================
#include "inputKeyboard.h"
#include "renderer.h"
#include "player.h"
#include "debugProc.h"
#include "camera.h"
#include "meshField.h"
#include "time.h"
#include "title.h"
#include <initializer_list>
#include "tutorial.h"
#include "game.h"
#include "inputGamepad.h"
#include "character.h"
#include "scene.h"
#include "modelCharacter.h"
#include "motion.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "3DBoxCollider.h"
#include "stone.h"
#include "wall.h"
#include "CylinderCollider.h"
#include "motion.h"
#include "Reflection.h"
#include "hitpoint.h"
#include "PolygonCollider.h"
#include "CapsuleCollider.h"
#include "sound.h"
#include "polyCollMana.h"
#include "shadow.h"

//==================================================================================================================
// マクロ定義
//==================================================================================================================
#define HEIGHT_CEILING	(400.0f)			// 天井の高さ

#define BLOWAWAYFORCE_SMASH		(100.0f)	// 吹き飛ばし力(スマッシュ攻撃)
#define BLOWAWAYFORCE_NORMAL	(8.0f)		// 吹き飛ばし力(通常攻撃)

#define TIME_LIFT_BEGIN			(60)		// 持ち上げ開始のモーション時間
#define TIME_MAX_DOWN			(60)		// 最大までダウンできる時間

//==================================================================================================================
// 静的メンバ変数の初期化
//==================================================================================================================
CHitPoint *CPlayer::m_pHitPoint = NULL;				// HP情報

//==================================================================================================================
// コンストラクタ
//==================================================================================================================
CPlayer::CPlayer(PRIORITY type = CScene::PRIORITY_PLAYER) : CCharacter(type)
{

}

//==================================================================================================================
// デストラクタ
//==================================================================================================================
CPlayer::~CPlayer()
{

}

//==================================================================================================================
// 初期化処理
//==================================================================================================================
void CPlayer::Init(void)
{
	// 要素の初期化
	m_bTrans = false;
	m_nCntState = 0;

	// 最大ポリゴン数までカウント
	for (int nCnt = 0; nCnt < 3; nCnt++)
	{
		m_bIn[nCnt] = false;
	}

	for (int nCnt = 0; nCnt < CStone::STONE_ID_MAX; nCnt++)
	{
		m_bGetStoneType[nCnt] = false;
	}

	// 初期化
	CCharacter::Init();
	CCharacter::SetModelType(m_type);

	// 当たり判定の設定
	this->m_nBoxColliderID = C3DBoxCollider::SetColliderInfo(&this->GetPos(), this, C3DBoxCollider::COLLIDER_SUB_NORMAL, C3DBoxCollider::ID_CHARACTER);

	m_pHitPoint = CHitPoint::Create(m_nPlayer, m_param.fMaxLife);
	m_pHitPoint->SetnPlayerNum(m_nPlayer);

	// 影生成
	m_pShadow = CShadow::Create();
}

//==================================================================================================================
// 終了処理
//==================================================================================================================
void CPlayer::Uninit(void)
{
	CCharacter::Uninit();

	m_pHitPoint = nullptr;	// 変数NULL
}

//==================================================================================================================
// 更新処理
//==================================================================================================================
void CPlayer::Update(void)
{
	CGame::GAMESTATE gameState = CManager::GetRenderer()->GetGame()->GetGameState();
	if (m_bBlowAway == false && m_bDaunted == false &&
		(gameState == CGame::GAMESTATE_NORMAL ||
			gameState == CGame::GAMESTATE_BEFORE))
		// 操作
		Control();

	// 更新
	CCharacter::Update();

	// 当たり判定
	Collision();

	// プレイヤーの番号設定
	SetnPlayer(m_nPlayer);

#ifdef _DEBUG
	char cText[8];
	sprintf(cText, "%d / ", m_pModelCharacter->GetAllFrame() - m_nAttackFrame);
	CDebugProc::Print(cText);
	sprintf(cText, "%d\n", m_pModelCharacter->GetAllFrame());
	CDebugProc::Print(cText);

	CDebugProc::Print("プレイヤーの位置 [%.4f][%.4f][%.4f]\n", m_pos.x, m_pos.y, m_pos.z);

	ShowDebugInfo();

	/*if (CManager::GetInputKeyboard()->GetKeyboardTrigger(DIK_2))
	m_bTrans = true;*/
#endif
}

//==================================================================================================================
// 描画処理
//==================================================================================================================
void CPlayer::Draw(void)
{
	CCharacter::Draw();
}

//==================================================================================================================
// 生成処理
//==================================================================================================================
CPlayer *CPlayer::Create(int nPlayer, CHARACTER_TYPE type)
{
	// シーン動的に確保
	CPlayer *pPlayer = new CPlayer(CScene::PRIORITY_PLAYER);

	// 失敗
	if (!pPlayer)
		return nullptr;

	// プレイヤーのキャラタイプを設定
	pPlayer->m_type = type;
	// プレイヤー番号の保存
	pPlayer->m_nPlayer = nPlayer;
	// 初期化
	pPlayer->Init();

	// 値を返す
	return pPlayer;
}

//==================================================================================================================
// 操作
//==================================================================================================================
void CPlayer::Control(void)
{
	CInputGamepad *pGamepad = CManager::GetInputGamepad(m_nPlayer);
	CInputKeyboard *pKeyboard = CManager::GetInputKeyboard();

	if (pGamepad->GetbConnect())
		ControlGamepad(pGamepad);
	else
		ControlKeyboard(pKeyboard);
}

//==================================================================================================================
// 当たり判定処理
//==================================================================================================================
void CPlayer::Collision(void)
{
	// 攻撃判定
	CollisionAttack();

	// 壁の取得
	CWall *pWall = CGame::GetWall();
	// 出力される交点
	D3DXVECTOR3 out_intersect = ZeroVector3;
	// 出力される法線
	D3DXVECTOR3 out_nor = ZeroVector3;
	// 壁との当たり判定
	if (pWall->Collision(&m_pos, &m_posOld, &out_intersect, &out_nor, m_bSmashBlowAway) == true)
	{
		// 反射フラグが立っているときかつ
		// 出力された法線がゼロじゃない時かつ
		// 出力された交点がゼロじゃない時
		if (m_bSmashBlowAway == true &&
			out_nor != ZeroVector3 &&
			out_intersect != ZeroVector3)
		{
			// ダメージ
			this->Damage(2);
			// 向きを決定
			this->m_rotDest.y = atan2f(out_nor.x, out_nor.z);
			// 回転を補間
			CKananLibrary::InterpolationFloat(m_rotDest.y);
			// 一瞬で向きを変える
			this->m_rot.y = this->m_rotDest.y;
			CReflection::GetPlaneReflectingAfterPosAndVec(&this->m_pos, &this->m_move, &out_intersect, &this->m_move, &out_nor);
		}
	}

	CPolyCollMana *pPolyCollMana = CGame::GetpolyCollMana();

	for (int nCntPolyColli = 0; nCntPolyColli < CPolygonCollider::GetNumPoly(CGame::GetStageType()); nCntPolyColli++)
	{
		// ポリゴンコライダーの取得
		CPolygonCollider*pPolyColli = pPolyCollMana->GetPolyColl(nCntPolyColli);

		// ポリゴンコライダーポインタがNULLだった時
		if (pPolyColli == NULL)
		{// スキップ
			continue;
		}

		// ポリゴンの範囲内にいるとき
		if (pPolyColli->Test3DInPolygon(&m_pos))
		{
			// 範囲内にいる
			m_bIn[nCntPolyColli] = true;
		}
		else
		{
			// 範囲内にいない
			m_bIn[nCntPolyColli] = false;
		}

		// ポリゴンコライダーの衝突判定
		if (pPolyColli->Collision(&m_pos, &m_posOld, &m_move, &out_intersect, &out_nor, m_bSmashBlowAway) == true)
		{
			if (m_bSmashBlowAway == false)
			{
#ifdef _DEBUG
				CDebugProc::Print("乗っている\n");
#endif
				// 地面に乗っていたら、移動量をなくす
				if (m_move.y <= -5.0f)
					m_move.y = -5.0f;
				// ジャンプ解除
				m_bJump = false;
				// ジャンプカウンタを初期化
				m_nCntJump = 0;
			}
			else
			{
				CReflection::GetPlaneReflectingAfterPosAndVec(&this->m_pos, &this->m_move, &out_intersect, &this->m_move, &out_nor);
			}
		}
	}

	// 当たり判定位置の更新
	C3DBoxCollider::ChangePosition(this->m_nBoxColliderID, this->m_pos, MYLIB_3DVECTOR_ZERO);
	// 当たり判定
	if (C3DBoxCollider::CollisionBox(this->m_nBoxColliderID, this->m_pos, m_move))
	{
		// ジャンプ解除
		m_bJump = false;
		// ジャンプカウンタを初期化
		m_nCntJump = 0;
	}

	// 最大ポリゴン数までカウント
	for (int nCntPolyColli = 0; nCntPolyColli < CPolygonCollider::GetNumPoly(CGame::GetStageType()); nCntPolyColli++)
	{
		// ポリゴンコライダーの取得
		CPolygonCollider*pPolyColli = pPolyCollMana->GetPolyColl(nCntPolyColli);

		// 範囲外のとき
		if (!m_bIn[nCntPolyColli])
		{
			// 影処理
			Shadow();
		}
		else
		{
			// 影位置設定
			m_pShadow->SetPos(D3DXVECTOR3(m_pos.x, pPolyColli->GetfHeight(), m_pos.z), m_move, m_bJump);
			
			break;
		}
	}

	// 高さ制限
	if (m_pos.y > HEIGHT_CEILING)
	{
		m_pos.y = HEIGHT_CEILING;
		m_move.y *= -1;
	}
}

//==================================================================================================================
// スマッシュ
//==================================================================================================================
void CPlayer::Smash(void)
{
	// 条件を設定
	m_bWalk = false;
	m_bAttack = true;

	// スマッシュ
	if (m_pModelCharacter->GetMotion() == CMotion::PLAYER_SMASH_CHARGE)
		m_pModelCharacter->SetMotion(CMotion::PLAYER_SMASH);
	// スマッシュチャージ
	else if (m_pModelCharacter->GetMotion() != CMotion::PLAYER_SMASH_CHARGE &&
		m_pModelCharacter->GetMotion() != CMotion::PLAYER_SMASH)
	{
		m_pModelCharacter->SetMotion(CMotion::PLAYER_SMASH_CHARGE);
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_SMASHCHARGE);
	}

	// 攻撃が当たったフラグをオフにする
	m_bAttakHit = false;
	// 攻撃フレームを設定
	m_nAttackFrame = m_pModelCharacter->GetAllFrame();
}

//==================================================================================================================
// 通常攻撃
//==================================================================================================================
void CPlayer::NormalAttack(void)
{
	int AllFrame = m_pModelCharacter->GetAllFrame();

	if (!m_bAttack && !m_bJump)
	{
		// 条件を設定
		m_bWalk = false;
		m_bAttack = true;
	}
	else if (m_bAttack)
	{
		if (AllFrame - m_nAttackFrame > AllFrame ||
			AllFrame - m_nAttackFrame < m_param.motionParam[(CCharaParam::PLAYER_ATTACK_TYPE)m_nAttackFlow - 1].CancelFrame.start)
			return;
	}

	// モーションの切り替え
	m_pModelCharacter->SetMotion((CMotion::MOTION_TYPE)(CMotion::PLAYER_ATTACK_0 + m_nAttackFlow));
	// 攻撃が当たったフラグをオフにする
	m_bAttakHit = false;
	// 攻撃フレームを設定
	m_nAttackFrame = m_pModelCharacter->GetAllFrame();
	// 攻撃の順番を設定
	m_nAttackFlow++;
	if (m_nAttackFlow >= 4)
		m_nAttackFlow = 0;

}

//==================================================================================================================
// ジャンプ
//==================================================================================================================
void CPlayer::Jump(void)
{
	// 条件を設定
	m_bJump = true;
	m_bWalk = false;

	// 移動値を設定
	m_move.y = m_param.moveParam.fJumpPower;
}

//==================================================================================================================
// 物持ち処理
//==================================================================================================================
void CPlayer::Lift(void)
{
	// 持ち上げていなければ、処理しない
	if (m_StateLift == STATE_NONE)
	{
		return;
	}

	if (m_StateLift == STATE_LIFT)
	{
		if (m_nCntState < TIME_LIFT_BEGIN)
		{
			m_nCntState++;
		}
		else
		{
			m_nCntState = 0;
			m_StateLift = STATE_LIFT_NEUTRAL;
		}
	}
}

//==================================================================================================================
// 影関係の更新処理
//==================================================================================================================
void CPlayer::Shadow(void)
{
	// 影位置設定
	m_pShadow->SetPos(D3DXVECTOR3(m_pos.x, C3DBoxCollider::GetHeight(), m_pos.z), m_move, m_bJump);
}

//==================================================================================================================
// モーション管理関数
//==================================================================================================================
void CPlayer::Motion(void)
{
	switch (m_stateStand)
	{
		// ニュートラル
	case STANDSTATE_NEUTRAL:
		MotionNeutral();
		break;
		// 怯み
	case STANDSTATE_DAUNTED:
		MotionDaunted();
		break;
		// 吹っ飛び
	case STANDSTATE_BLOWAWAY:
		MotionBlowAway();
		break;
		// ダウン
	case STANDSTATE_DOWN:
		MotionDown();
		break;
		// 起き上がり
	case STANDSTATE_GETUP:
		MotionGetUp();
		break;
		// 歩行
	case STANDSTATE_WALK:
		MotionWalk();
		break;
		// ジャンプ
	case STANDSTATE_JUMP:
		MotionJump();
		break;
		// 攻撃
	case STANDSTATE_ATTACK:
		MotionAttack();
		break;
		// 持ち上げ
	case STANDSTATE_LIFT:
		MotionLift();
		break;
	}
}

//==================================================================================================================
// ニュートラルモーション
//==================================================================================================================
void CPlayer::MotionNeutral(void)
{
	// モーション切替
	if (m_pModelCharacter->GetMotion() != CMotion::PLAYER_NEUTRAL)
	{
		m_pModelCharacter->SetMotion(CMotion::PLAYER_NEUTRAL);
		m_nCntState = 0;
	}
}

//==================================================================================================================
// 歩きモーション
//==================================================================================================================
void CPlayer::MotionWalk(void)
{
	// モーション切替
	if (m_pModelCharacter->GetMotion() != CMotion::PLAYER_WALK)
	{
		m_pModelCharacter->SetMotion(CMotion::PLAYER_WALK);
		m_nCntState = 0;
	}
}

//==================================================================================================================
// 持ち上げモーション
//==================================================================================================================
void CPlayer::MotionLift(void)
{
	switch (m_StateLift)
	{
		// 持ち上げ
	case STATE_LIFT:
		m_pModelCharacter->SetMotion(CMotion::PLAYER_LIFT);
		break;
	case STATE_LIFT_NEUTRAL:
		m_pModelCharacter->SetMotion(CMotion::PLAYER_LIFT_NEUTRAL);
		break;
	case STATE_WALK:
		m_pModelCharacter->SetMotion(CMotion::PLAYER_LIFT);
		break;
	case STATE_THROW:
		break;
	}
}

//==================================================================================================================
// ジャンプモーション
//==================================================================================================================
void CPlayer::MotionJump(void)
{
	// 攻撃の状態を初期化
	m_nAttackFlow = 0;
	// ジャンプカウンタを加算
	m_nCntJump++;
	// 最初はジャンプモーション
	if (m_nCntJump <= 15)
		m_pModelCharacter->SetMotion(CMotion::PLAYER_JUMP);
	// 以降は落下モーション
	else
		m_pModelCharacter->SetMotion(CMotion::PLAYER_FALL);
}

//==================================================================================================================
// ダウンモーション
//==================================================================================================================
void CPlayer::MotionDown(void)
{
	// モーション切替
	if (m_pModelCharacter->GetMotion() != CMotion::PLAYER_DOWN)
	{
		m_pModelCharacter->SetMotion(CMotion::PLAYER_DOWN);
		m_nCntState = 0;
	}

	// カウント加算
	m_nCntState++;

	// 一定時間内に、操作でアクティブな起き上がり
	if (m_nCntState < TIME_MAX_DOWN &&
		(CKananLibrary::GetMoveByGamepad(CManager::GetInputGamepad(m_nPlayer)) ||
			CKananLibrary::GetMoveByKeyboard(CManager::GetInputKeyboard(), m_nPlayer)))
		m_stateStand = STANDSTATE_GETUP_ACTIVE;
	// 一定時間後に、起き上がり
	else if (m_nCntState >= TIME_MAX_DOWN)
		m_stateStand = STANDSTATE_GETUP;
	// 処理の終了
	else
		return;

	// カウント初期化
	m_nCntState = 0;
}

//==================================================================================================================
// 怯みモーション
//==================================================================================================================
void CPlayer::MotionDaunted(void)
{
	// モーション切替
	if (m_pModelCharacter->GetMotion() != CMotion::PLAYER_DAUNTED)
	{
		m_pModelCharacter->SetMotion(CMotion::PLAYER_DAUNTED);
		m_nCntState = 0;
	}



	// 後隙フレーム減算
	m_nCntGap--;
	// 攻撃の状態を初期化
	m_nAttackFlow = 0;
	// 怯み終了
	if (m_nCntGap <= 0)
	{
		// 怯み解除
		m_bDaunted = false;
		// 後隙フレームを初期化
		m_nCntGap = 0;
	}
}

//==================================================================================================================
// 吹っ飛びモーション
//==================================================================================================================
void CPlayer::MotionBlowAway(void)
{
	// 吹っ飛び中
	if (m_bBlowAway)
	{
		// 地面に着く
		if (m_move.y <= -3.0f)
		{
			// 吹き飛び終了
			m_bBlowAway = false;
			// ダウン開始
			m_bDown = true;
		}
	}

	// スマッシュ吹き飛び中
	if (m_bSmashBlowAway)
	{
		// 吹き飛びの威力が落ちる
		if (abs(m_move.x) <= 5.0f &&
			abs(m_move.z) <= 5.0f &&
			abs(m_move.y) <= 5.0f)
		{
			// 吹き飛び終了
			m_bSmashBlowAway = false;
			// ダウン開始
			m_bDown = true;
		}
	}
}

//==================================================================================================================
// 攻撃モーション
//==================================================================================================================
void CPlayer::MotionAttack(void)
{
	// 攻撃フレームを減算
	m_nAttackFrame--;
	// 攻撃終了後
	if (m_nAttackFrame <= 0)
	{
		// 攻撃解除
		m_bAttack = false;
		// 攻撃の状態を初期化
		m_nAttackFlow = 0;
		// 攻撃フレームを初期化
		m_nAttackFrame = 0;
	}
}

//==================================================================================================================
// 起き上がりモーション
//==================================================================================================================
void CPlayer::MotionGetUp(void)
{
}

//==================================================================================================================
// 攻撃判定
//==================================================================================================================
void CPlayer::CollisionAttack(void)
{
	// 違うプレイヤーの取得
	CPlayer *pAnother = GetAnotherPlayer();

	// 違うプレイヤーが攻撃を当てたフラグが立ってない時
	if (pAnother->m_bAttakHit == false)
	{
		// 別のプレイヤーのモーションを比較
		switch (pAnother->m_pModelCharacter->GetMotion())
		{
		case CMotion::PLAYER_ATTACK_0:
			// 別のプレイヤーが攻撃0をしている時
			this->AnotherPlayerAttack0(pAnother);
			break;
		case CMotion::PLAYER_ATTACK_1:
			// 別のプレイヤーが攻撃1をしている時
			this->AnotherPlayerAttack1(pAnother);
			break;
		case CMotion::PLAYER_ATTACK_2:
			// 別のプレイヤーが攻撃2をしている時
			this->AnotherPlayerAttack2(pAnother);
			break;
		case CMotion::PLAYER_ATTACK_3:
			// 別のプレイヤーが攻撃3をしている時
			this->AnotherPlayerAttack3(pAnother);
			break;
		case CMotion::PLAYER_SMASH:
			// 別のプレイヤーがスマッシュ攻撃しているとき
			this->AnotherPlayerSmash(pAnother);
			break;
		}
	}
}

//==================================================================================================================
// 攻撃当てる準備かできているか
//==================================================================================================================
bool CPlayer::ReadyToHit(const int &nCapColliID)
{
	// 違うプレイヤーが攻撃を当てたフラグが立ってない時
	if (this->m_bAttakHit == false)
	{
		// 別のプレイヤーのモーションを比較
		switch (this->m_pModelCharacter->GetMotion())
		{
		case CMotion::PLAYER_ATTACK_0: return HitConditionAttack0(nCapColliID);
		case CMotion::PLAYER_ATTACK_1: return HitConditionAttack1(nCapColliID);
		case CMotion::PLAYER_ATTACK_2: return HitConditionAttack2(nCapColliID);
		case CMotion::PLAYER_ATTACK_3: return HitConditionAttack3(nCapColliID);
		case CMotion::PLAYER_SMASH:    return HitConditionSmash(nCapColliID);
		}
	}
	return false;
}

//==================================================================================================================
//　攻撃0を当てる条件
//==================================================================================================================
bool CPlayer::HitConditionAttack0(const int &nCapColliID)
{
	return (nCapColliID == CCapsuleCollider::TYPEID_FOREARM_L) || (nCapColliID == CCapsuleCollider::TYPEID_UPPERARM_L);
}

//==================================================================================================================
//　攻撃1を当てる条件
//==================================================================================================================
bool CPlayer::HitConditionAttack1(const int &nCapColliID)
{
	return (nCapColliID == CCapsuleCollider::TYPEID_FOREARM_R) || (nCapColliID == CCapsuleCollider::TYPEID_UPPERARM_R);
}

//==================================================================================================================
//　攻撃2を当てる条件
//==================================================================================================================
bool CPlayer::HitConditionAttack2(const int &nCapColliID)
{
	return (nCapColliID == CCapsuleCollider::TYPEID_FOREARM_L) || (nCapColliID == CCapsuleCollider::TYPEID_UPPERARM_L);
}

//==================================================================================================================
//　攻撃3を当てる条件
//==================================================================================================================
bool CPlayer::HitConditionAttack3(const int &nCapColliID)
{
	return (nCapColliID == CCapsuleCollider::TYPEID_FOREARM_R) || (nCapColliID == CCapsuleCollider::TYPEID_UPPERARM_R);
}

//==================================================================================================================
//　スマッシュ攻を当てる条件
//==================================================================================================================
bool CPlayer::HitConditionSmash(const int &nCapColliID)
{
	return (nCapColliID == CCapsuleCollider::TYPEID_FOREARM_R) || (nCapColliID == CCapsuleCollider::TYPEID_UPPERARM_R);
}

//==================================================================================================================
// 違うプレイヤーの取得
//==================================================================================================================
CPlayer * CPlayer::GetAnotherPlayer(void)
{
	return (this->m_nPlayer == PLAYER_ONE) ? CGame::GetPlayer(PLAYER_TWO) : CGame::GetPlayer(PLAYER_ONE);
}

//==================================================================================================================
// ゲームパッドの操作
//==================================================================================================================
void CPlayer::ControlGamepad(CInputGamepad * pGamepad)
{
	float fValueX, fValueY;	// ゲームパッドのスティック情報の取得用

							// 左スティック取得
	pGamepad->GetStickLeft(&fValueX, &fValueY);

	// 何も入力されていなければ、処理しない
	if (FAILED(CKananLibrary::GetMoveByGamepad(pGamepad)) &&
		fValueX == 0 && fValueY == 0)
	{
		m_bWalk = false;
		return;
	}

	if (CGame::GetGameState() == CGame::GAMESTATE_NORMAL)
	{
		// 変身中、スマッシュ入力
		if (m_bTrans &&
			(pGamepad->GetTrigger(CInputGamepad::JOYPADKEY_B)))
			// スマッシュ
			Smash();

		// スマッシュ系モーション中は以降の処理をしない
		if (m_pModelCharacter->GetMotion() == CMotion::PLAYER_SMASH_CHARGE ||
			m_pModelCharacter->GetMotion() == CMotion::PLAYER_SMASH)
			// 処理を終える
			return;

		// 攻撃入力
		if (pGamepad->GetTrigger(CInputGamepad::JOYPADKEY_X) &&
			(m_pModelCharacter->GetMotion() != CMotion::PLAYER_SMASH_CHARGE &&
				m_pModelCharacter->GetMotion() != CMotion::PLAYER_SMASH))
		{
			// 通常攻撃
			NormalAttack();
			// 処理を終える
			return;
		}
	}

	CCamera *pCamera = CManager::GetRenderer()->GetGame()->GetCamera();	// カメラ取得

	D3DXVECTOR3 rotDest = GetRotDest();				// 目的の向きを格納する変数
	D3DXVECTOR3 vecCamera = pCamera->GetVec();		// カメラの向いている方向の取得
	float		CameraRotY = pCamera->GetRotY();	// カメラのY軸回転の取得
	float		fSpeed = 0.0f;						// プレイヤーの速度
	float		fAngle;								// スティック角度の計算用変数

	if (!m_bJump && !m_bAttack &&
		pGamepad->GetTrigger(CInputGamepad::JOYPADKEY_A))
	{
		// ジャンプ
		Jump();
	}

	if (m_bAttack)
	{
		m_bWalk = false;
		return;
	}

	if (m_bDown)
	{
		m_bWalk = false;
		return;
	}

	// 角度の計算して補正
	fAngle = atan2f(fValueX, fValueY);
	CKananLibrary::InterpolationFloat(fAngle);

	// スティックの倒れ具合でスピードを決定
	if (abs(fValueX) > abs(fValueY))
		fSpeed = (abs(fValueX));		// 横の倒れ具合
	else
		fSpeed = (abs(fValueY));		// 縦の倒れ具合

										// スティックの角度によってプレイヤー移動
	m_move.x += sinf(fAngle + CameraRotY) * fSpeed * m_param.moveParam.fRunSpeed;
	m_move.z += cosf(fAngle + CameraRotY) * fSpeed * m_param.moveParam.fRunSpeed;

	// 目的の向きを決定
	if (fValueX != 0 || fValueY != 0)
	{
		rotDest.y = D3DX_PI + fAngle + CameraRotY;
		m_bWalk = true;
	}

	if (!m_bJump)
	{
		if (!m_bWalk)
			// 歩き始めはモーションリセット
			m_pModelCharacter->ResetMotion();
		// 歩いている
		m_bWalk = true;
	}

	// 回転の補正
	CKananLibrary::InterpolationRot(&rotDest);

	// 目的の回転の設定
	SetRotDest(rotDest);
}

//==================================================================================================================
// キーボードの操作
//==================================================================================================================
void CPlayer::ControlKeyboard(CInputKeyboard * pKeyboard)
{
	// 入力されていなければ処理を終える
	if (FAILED(CKananLibrary::GetMoveByKeyboard(pKeyboard, m_nPlayer)))
	{
		m_bWalk = false;
		return;
	}

	// 変数宣言
	CCamera *pCamera = CManager::GetRenderer()->GetGame()->GetCamera();	// カメラ取得
	D3DXVECTOR3 rotDest = GetRotDest();										// 目的の向きを格納する変数
	float		CameraRotY = pCamera->GetRotY();								// カメラのY軸回転の取得

	if (CGame::GetGameState() == CGame::GAMESTATE_NORMAL)
	{
		// 変身中、スマッシュ入力
		if (m_bTrans &&
			((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardTrigger(ONE_SMASH)) ||
			(m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardTrigger(TWO_SMASH))))
			// スマッシュ
			Smash();

		// スマッシュ系モーション中は以降の処理をしない
		if (m_pModelCharacter->GetMotion() == CMotion::PLAYER_SMASH_CHARGE ||
			m_pModelCharacter->GetMotion() == CMotion::PLAYER_SMASH)
			// 処理を終える
			return;

		// 攻撃入力
		if ((((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardTrigger(ONE_ATTACK)) ||
			(m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardTrigger(TWO_ATTACK))) &&
			m_pModelCharacter->GetMotion() != CMotion::PLAYER_SMASH_CHARGE &&
			m_pModelCharacter->GetMotion() != CMotion::PLAYER_SMASH))
		{
			// 通常攻撃
			NormalAttack();
			// 処理を終える
			return;
		}
	}

	if (m_StateLift == STATE_NONE &&
		((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardTrigger(ONE_LIFT)) ||
		(m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardTrigger(TWO_LIFT))))
	{
		m_bLift = true;
		m_StateLift = STATE_LIFT;
	}

	// 物持ち上げの処理
	Lift();

	if (!m_bJump && !m_bAttack &&
		(m_nPlayer == PLAYER_ONE && (pKeyboard->GetKeyboardTrigger(ONE_JUMP)) ||
			m_nPlayer == PLAYER_TWO && (pKeyboard->GetKeyboardTrigger(TWO_JUMP))))
	{
		// ジャンプ
		Jump();
	}

	if (m_bAttack)
	{
		m_bWalk = false;
		return;
	}

	if (m_bDown)
	{
		m_bWalk = false;
		return;
	}

	// Aキー長押し
	if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_LEFT)) ||
		m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_LEFT))
	{
		// Wキー長押し
		if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_UP)) ||
			m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_UP))
		{
			// 左上移動
			m_move.x += sinf(-D3DX_PI * 0.75f - CameraRotY) * m_param.moveParam.fRunSpeed;
			m_move.z -= cosf(-D3DX_PI * 0.75f - CameraRotY) * m_param.moveParam.fRunSpeed;
			// 目的の向きを決定
			rotDest.y = D3DX_PI * 0.75f + CameraRotY;
		}
		// Sキー長押し
		else if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_DOWN)) ||
			m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_DOWN))
		{
			// 左下移動
			m_move.x += sinf(-D3DX_PI * 0.25f - CameraRotY) * m_param.moveParam.fRunSpeed;
			m_move.z -= cosf(-D3DX_PI * 0.25f - CameraRotY) * m_param.moveParam.fRunSpeed;
			// 目的の向きを決定
			rotDest.y = D3DX_PI * 0.25f + CameraRotY;
		}
		// Aキーのみ
		else
		{
			// 左移動
			m_move.x += sinf(-D3DX_PI * 0.5f - CameraRotY) * m_param.moveParam.fRunSpeed;
			m_move.z -= cosf(-D3DX_PI * 0.5f - CameraRotY) * m_param.moveParam.fRunSpeed;
			// 目的の向きを決定
			rotDest.x = 0.0f;
			rotDest.y = D3DX_PI * 0.5f + CameraRotY;
		}
	}
	// Dキー長押し
	else if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_RIGHT)) ||
		m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_RIGHT))
	{
		// Wキー長押し
		if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_UP)) ||
			m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_UP))
		{
			// 右上移動
			m_move.x += sinf(D3DX_PI * 0.75f - CameraRotY) * m_param.moveParam.fRunSpeed;
			m_move.z -= cosf(D3DX_PI * 0.75f - CameraRotY) * m_param.moveParam.fRunSpeed;
			// 目的の向きを決定
			rotDest.y = -D3DX_PI * 0.75f + CameraRotY;
		}
		// Sキー長押し
		else if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_DOWN)) ||
			m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_DOWN))
		{
			// 右下移動
			m_move.x += sinf(D3DX_PI * 0.25f - CameraRotY) * m_param.moveParam.fRunSpeed;
			m_move.z -= cosf(D3DX_PI * 0.25f - CameraRotY) * m_param.moveParam.fRunSpeed;
			// 目的の向きを決定
			rotDest.y = -D3DX_PI * 0.25f + CameraRotY;
		}
		// Dキーのみ
		else
		{
			// 右移動
			m_move.x += sinf(D3DX_PI * 0.5f - CameraRotY) * m_param.moveParam.fRunSpeed;
			m_move.z -= cosf(D3DX_PI * 0.5f - CameraRotY) * m_param.moveParam.fRunSpeed;
			// 目的の向きを決定
			rotDest.y = -D3DX_PI * 0.5f + CameraRotY;
		}
	}
	// Wキー長押し
	else if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_UP)) ||
		m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_UP))
	{
		// 上移動
		m_move.x += sinf(D3DX_PI * 1.0f - CameraRotY) * m_param.moveParam.fRunSpeed;
		m_move.z -= cosf(D3DX_PI * 1.0f - CameraRotY) * m_param.moveParam.fRunSpeed;
		// 目的の向きを決定
		rotDest.y = -D3DX_PI * 1.0f + CameraRotY;
	}
	// Sキー長押し
	else if ((m_nPlayer == PLAYER_ONE && pKeyboard->GetKeyboardPress(ONE_DOWN)) ||
		m_nPlayer == PLAYER_TWO && pKeyboard->GetKeyboardPress(TWO_DOWN))
	{
		// 下移動
		m_move.x += sinf(D3DX_PI * 0.0f - CameraRotY) * m_param.moveParam.fRunSpeed;
		m_move.z -= cosf(D3DX_PI * 0.0f - CameraRotY) * m_param.moveParam.fRunSpeed;
		// 目的の向きを決定
		rotDest.y = CameraRotY;
	}

	if (!m_bJump)
	{
		if (!m_bWalk)
			// 歩き始めはモーションリセット
			m_pModelCharacter->ResetMotion();
		// 歩いている
		m_bWalk = true;
	}

	// 回転の補正
	CKananLibrary::InterpolationRot(&rotDest);

	// 目的の回転の設定
	SetRotDest(rotDest);
}

//==================================================================================================================
// ストーンの取得判定
//==================================================================================================================
void CPlayer::CatchStone(CStone *pStone)
{
	CGame::RemoveNumStone(pStone->GetIndexPos());
	// 取得ストーンのタイプを有効
	int nStoneID = pStone->GetStoneID();
	m_bGetStoneType[nStoneID] = true;
	// ストーンの取得
	pStone->Catch();

	// ストーンの取得数を加算
	m_nNumStone++;
	// 3つ取得している
	if (m_nNumStone >= 3)
	{
		// 変身
		this->m_bTrans = true;
		// SE
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_TRANSFORM);
		// BGM
		CRenderer::GetSound()->StopSound(CSound::SOUND_LABEL_BGM_GAME);
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_BGM_TRANS);
	}
	// それ以外
	else
	{
		// SE
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_GETSTONE);
	}
}

//==================================================================================================================
// 別のプレイヤーが攻撃0している時
//==================================================================================================================
void CPlayer::AnotherPlayerAttack0(CPlayer * pAnother)
{
	// if (pAnother->m_pCapColi[CCharacter::COLLIPARTS_FOREARM_L]->Collision(this->m_nBoxColliderID) == true ||
	// 	pAnother->m_pCapColi[CCharacter::COLLIPARTS_UPPERARM_L]->Collision(this->m_nBoxColliderID) == true)
	// {
	// 	// ダメージを受ける
	// 	TakeDamage();
	// 	// 当てたフラグを立てる
	// 	pAnother->m_bAttakHit = true;
	// }
}

//==================================================================================================================
// 別のプレイヤーが攻撃1している時
//==================================================================================================================
void CPlayer::AnotherPlayerAttack1(CPlayer * pAnother)
{
	//if (pAnother->m_pCyliColi[CCharacter::COLLIPARTS_FOREARM_R]->Collision(this->m_nBoxColliderID) == true ||
	//	pAnother->m_pCyliColi[CCharacter::COLLIPARTS_UPPERARM_R]->Collision(this->m_nBoxColliderID) == true)
	//{
	// ダメージを受ける
	//	TakeDamage();
	//	// 当てたフラグを立てる
	//	pAnother->m_bAttakHit = true;
	//}
}

//==================================================================================================================
// 別のプレイヤーが攻撃2している時
//==================================================================================================================
void CPlayer::AnotherPlayerAttack2(CPlayer * pAnother)
{
	//if (pAnother->m_pCyliColi[CCharacter::COLLIPARTS_FOREARM_L]->Collision(this->m_nBoxColliderID) == true ||
	//	pAnother->m_pCyliColi[CCharacter::COLLIPARTS_UPPERARM_L]->Collision(this->m_nBoxColliderID) == true)
	//{
	// ダメージを受ける
	//	TakeDamage();
	//	// 当てたフラグを立てる
	//	pAnother->m_bAttakHit = true;
	//}
}

//==================================================================================================================
// 別のプレイヤーが攻撃3している時
//==================================================================================================================
void CPlayer::AnotherPlayerAttack3(CPlayer * pAnother)
{
	//if (pAnother->m_pCyliColi[CCharacter::COLLIPARTS_FOREARM_R]->Collision(this->m_nBoxColliderID) == true ||
	//	pAnother->m_pCyliColi[CCharacter::COLLIPARTS_UPPERARM_R]->Collision(this->m_nBoxColliderID) == true)
	//{
	//	// ダメージ
	//	this->Damage(2);
	//	// 変身中以外は吹き飛ぶ
	//	if (!m_bTrans)
	//	{
	//		// 吹き飛び
	//		BlowAway(pAnother, 0.5f, BLOWAWAYFORCE_NORMAL);
	//		// 吹き飛びを有効
	//		m_bBlowAway = true;
	//		if (m_nNumStone > 0)
	//		{
	//			// 所持ストーンを一つ減らすn
	//			m_nNumStone--;
	//			// 減らしたストーンを即生成
	//			CGame::AppearStone();
	//		}
	//	}
	//	// 当てたフラグを立てる
	//	pAnother->m_bAttakHit = true;
	//}
}

//==================================================================================================================
// 別のプレイヤーがスマッシュ攻撃している時
//==================================================================================================================
void CPlayer::AnotherPlayerSmash(CPlayer * pAnother)
{
	//// シリンダーコライダーの衝突判定
	//if (pAnother->m_pCyliColi[CCharacter::COLLIPARTS_FOREARM_R]->Collision(this->m_nBoxColliderID) == true ||
	//	pAnother->m_pCyliColi[CCharacter::COLLIPARTS_UPPERARM_R]->Collision(this->m_nBoxColliderID) == true)
	//{
	//	// ダメージ
	//	this->Damage(2);
	//	// 変身中以外は吹き飛ぶ
	//	BlowAway(pAnother, 0.5f, BLOWAWAYFORCE_SMASH);
	//	// スマッシュによる吹き飛びを実行
	//	m_bSmashBlowAway = true;
	//	// 当てたフラグを立てる
	//	pAnother->m_bAttakHit = true;
	//}
}

//==================================================================================================================
//ダメージを受ける
//==================================================================================================================
void CPlayer::TakeDamage(CPlayer * pAnother)
{
	// ダメージ
	this->Damage(2);
	if (!m_bTrans)
		// 怯み
		this->Daunted(20);

	// 当てたフラグを立てる
	pAnother->m_bAttakHit = true;
}

//==================================================================================================================
// 攻撃3のダメージを受ける
//==================================================================================================================
void CPlayer::TakeAttack3Damage(CPlayer * pAnother)
{
	// ダメージ
	this->Damage(2);
	// 変身中以外は吹き飛ぶ
	if (!m_bTrans)
	{
		// 吹き飛び
		BlowAway(pAnother, 0.5f, BLOWAWAYFORCE_NORMAL);
		// 吹き飛びを有効
		m_bBlowAway = true;
		if (m_nNumStone > 0)
		{
			// 所持ストーンを一つ減らす
			m_nNumStone--;
			// 再配置できるようストーンを使用されていない状態にする
			CGame::RemoveTypeStone(CKananLibrary::DecideRandomValue(m_nNumStone + 1, m_bGetStoneType));
			// 減らしたストーンを即生成
			CGame::AppearStone();
		}
	}
	// 当てたフラグを立てる
	pAnother->m_bAttakHit = true;
}

//==================================================================================================================
// スマッシュ攻撃のダメージを受ける
//==================================================================================================================
void CPlayer::TakeSmashDamage(CPlayer * pAnother)
{
	// ダメージ
	this->Damage(2);
	// 変身中以外は吹き飛ぶ
	BlowAway(pAnother, 0.5f, BLOWAWAYFORCE_SMASH);
	// スマッシュによる吹き飛びを実行
	m_bSmashBlowAway = true;
	// 当てたフラグを立てる
	pAnother->m_bAttakHit = true;
}


//==================================================================================================================
// 
//==================================================================================================================
void CPlayer::SetHitSound()
{
	switch (m_pModelCharacter->GetMotion())
	{
	case CMotion::PLAYER_ATTACK_0:
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_HIT1);
		break;
	case CMotion::PLAYER_ATTACK_1:
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_HIT1);
		break;
	case CMotion::PLAYER_ATTACK_2:
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_HIT2);
		break;
	case CMotion::PLAYER_ATTACK_3:
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_HIT3);
		break;
	case CMotion::PLAYER_SMASH:
		CRenderer::GetSound()->PlaySound(CSound::SOUND_LABEL_SE_SMASHHIT);
		break;
	}

}

//==================================================================================================================
// プレイヤー番号設定処理
//==================================================================================================================
void CPlayer::SetnPlayer(int nPlayerNum)
{
	m_nPlayer = nPlayerNum;
}

//==================================================================================================================
// 吹き飛ぶ
//==================================================================================================================
inline bool CPlayer::BlowAway(CPlayer *pAnother, const float MoveVecY, const float fBlowAwayForce)
{
	// NULLだった時処理しない
	if (pAnother == NULL)
	{
		return false;
	}
	// 変数宣言
	D3DXVECTOR3 MoveVec;	// 移動ベクトル

	MoveVec.x = sinf(pAnother->m_rot.y + D3DX_PI);
	MoveVec.y = MoveVecY;
	MoveVec.z = cosf(pAnother->m_rot.y + D3DX_PI);

	// ぶっ飛びモーション
	m_pModelCharacter->ResetMotion();
	m_pModelCharacter->SetMotion(CMotion::PLAYER_BLOWAWAY);
	// 向きを決定
	m_rotDest.y = pAnother->m_rot.y + D3DX_PI;
	// 回転の補間
	CKananLibrary::InterpolationFloat(m_rotDest.y);
	// 一瞬で向きを変える
	m_rot.y = m_rotDest.y;

	// 移動値に加算
	this->m_move.x = MoveVec.x * fBlowAwayForce;
	this->m_move.z = MoveVec.z * fBlowAwayForce;
	this->m_move.y = MoveVec.y * fBlowAwayForce;

	return true;
}

#ifdef _DEBUG
//==================================================================================================================
// ImGuiの更新
//==================================================================================================================
void CPlayer::ShowDebugInfo()
{
	char cHead[8];
	sprintf(cHead, "Player%d", m_nPlayer);			// プレイヤー番号を文字列に追加

	if (ImGui::CollapsingHeader(cHead))
	{
		int nAllFrame = m_pModelCharacter->GetAllFrame();
		// 情報の表示
		CKananLibrary::ShowOffsetInfo(GetPos(), GetRot(), GetMove());
		ImGui::Text("nLife       : %f", m_nLife);
		ImGui::Text("bJump       : %d", m_bJump);
		ImGui::Text("bWalk       : %d", m_bWalk);
		ImGui::Text("bDown       : %d", m_bDown);
		ImGui::Text("bAttack     : %d", m_bAttack);
		if (m_bAttack)
		{
			ImGui::Text("AttackFlow  : %d", m_nAttackFlow);
			ImGui::Text("AttackFrame : %d / %d", nAllFrame - m_nAttackFrame, nAllFrame);
		}
		ImGui::Text("GetNumStone : %d", m_nNumStone);
		if (m_bTrans)
			ImGui::Text("TransTime   : %d", TIME_TRANS - m_nCntTrans);
	}
}
#endif
