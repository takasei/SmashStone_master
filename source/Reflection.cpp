//*************************************************************************************************************
//
// 反射の処理[Reflection.cpp]
// Author : Sekine Ikuto
//
//*************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------
// インクルードファイル
//-------------------------------------------------------------------------------------------------------------
#include "Reflection.h"
#include "3DParticle.h"
#include "CharEffectOffset.h"

//-------------------------------------------------------------------------------------------------------------
// マクロ定義
//-------------------------------------------------------------------------------------------------------------
#define REFLECTION_RATE		(0.9f)

//-------------------------------------------------------------------------------------------------------------
// 平面の反射後の位置とベクトルを算出する
//-------------------------------------------------------------------------------------------------------------
void CReflection::GetPlaneReflectingAfterPosAndVec(
	D3DXVECTOR3 *pOutPos,
	D3DXVECTOR3 *pOutMoveVec,
	D3DXVECTOR3 *pPos,
	D3DXVECTOR3 *pMove,
	D3DXVECTOR3 *pNormal)
{
	// 変数宣言
	D3DXVECTOR3 PlaneNom;	// 平面の法線ベクトル
	D3DXVECTOR3 ReflecVec;	// 反射のベクトル

	// 正規化
	D3DXVec3Normalize(&PlaneNom, pNormal);

	// 内積で反射のベクトルを計算
	ReflecVec = D3DXVec3Dot(&PlaneNom, pMove)*PlaneNom;

	// 反射後の速度ベクトルを計算
	*pOutMoveVec = *pMove - (1 + REFLECTION_RATE)*ReflecVec;

	// 反射後の位置を計算
	*pOutPos = *pPos + *pOutMoveVec;
	CCharEffectOffset::Set(pPos, CCharEffectOffset::STR_ダンッ);

	C3DParticle::Set(pPos, pNormal, C3DParticle::OFFSETNAME::HITREFLECTION);
}
