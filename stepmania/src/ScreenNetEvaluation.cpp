#include "global.h"

#if !defined(WITHOUT_NETWORKING)
#include "ScreenNetEvaluation.h"
#include "ThemeManager.h"
#include "GameState.h"
#include "RageLog.h"

static const int NUM_SCORE_DIGITS = 9;

#define USERSBG_WIDTH		THEME->GetMetricF("ScreenNetEvaluation","UsersBGWidth")
#define USERSBG_HEIGHT		THEME->GetMetricF("ScreenNetEvaluation","UsersBGHeight")
#define USERSBG_COMMAND		THEME->GetMetricA("ScreenNetEvaluation","UsersBGCommand")

#define USERDX			THEME->GetMetricF("ScreenNetEvaluation","UserDX")
#define USERDY			THEME->GetMetricF("ScreenNetEvaluation","UserDY")

#define MAX_COMBO_NUM_DIGITS	THEME->GetMetricI("ScreenEvaluation","MaxComboNumDigits")

static AutoScreenMessage( SM_GotEval ) 

REGISTER_SCREEN_CLASS( ScreenNetEvaluation );

void ScreenNetEvaluation::Init()
{
	ScreenEvaluation::Init();

	m_bHasStats = false;
	m_iCurrentPlayer = 0;

	FOREACH_ENUM( PlayerNumber, pn )
		if( GAMESTATE->IsPlayerEnabled(pn) )
			m_pActivePlayer = pn;

	if( m_pActivePlayer == PLAYER_1 )
		m_iShowSide = 2;
	else
		m_iShowSide = 1;

	m_rectUsersBG.SetWidth( USERSBG_WIDTH );
	m_rectUsersBG.SetHeight( USERSBG_HEIGHT );
	m_rectUsersBG.RunCommands( USERSBG_COMMAND );
	// XXX: The name should be ssprintf( "UsersBG%d", m_iShowSide ) and then
	// then LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND should be used.
	m_rectUsersBG.SetName( "UsersBG" );
	
	m_rectUsersBG.SetXY(
		THEME->GetMetricF("ScreenNetEvaluation",ssprintf("UsersBG%dX",m_iShowSide)),
		THEME->GetMetricF("ScreenNetEvaluation",ssprintf("UsersBG%dY",m_iShowSide)) );
	LOAD_ALL_COMMANDS_AND_ON_COMMAND( m_rectUsersBG );

	this->AddChild( &m_rectUsersBG );

	RedoUserTexts();

	NSMAN->ReportNSSOnOff( 5 );
}

void ScreenNetEvaluation::RedoUserTexts()
{
	m_iActivePlayers = NSMAN->m_ActivePlayers;

	//If unnecessary, just don't do this function.
	if ( m_iActivePlayers == (int)m_textUsers.size() )
		return;

	for( unsigned int i=0; i < m_textUsers.size(); ++i )
		this->RemoveChild( &m_textUsers[i] );

	float cx = THEME->GetMetricF("ScreenNetEvaluation",ssprintf("User%dX",m_iShowSide));
	float cy = THEME->GetMetricF("ScreenNetEvaluation",ssprintf("User%dY",m_iShowSide));
	
	m_iCurrentPlayer = 0;
	m_textUsers.clear();
	m_textUsers.resize(m_iActivePlayers);

	for( int i=0; i<m_iActivePlayers; ++i )
	{
		m_textUsers[i].LoadFromFont( THEME->GetPathF(m_sName,"names") );
		m_textUsers[i].SetName( "User" );
		m_textUsers[i].SetShadowLength( 1 );
		m_textUsers[i].SetXY( cx, cy );

		this->AddChild( &m_textUsers[i] );
		ActorUtil::LoadAllCommands( m_textUsers[i], m_sName );
		cx+=USERDX;
		cy+=USERDY;
	}
}

void ScreenNetEvaluation::MenuLeft( const InputEventPlus &input )
{
	MenuUp( input );
}

void ScreenNetEvaluation::MenuUp( const InputEventPlus &input )
{
	if( m_iActivePlayers == 0 || !m_bHasStats )
		return;

	COMMAND( m_textUsers[m_iCurrentPlayer], "DeSel" );
	m_iCurrentPlayer = (m_iCurrentPlayer + m_iActivePlayers - 1) % m_iActivePlayers;
	COMMAND( m_textUsers[m_iCurrentPlayer], "Sel" );
	UpdateStats();
}

void ScreenNetEvaluation::MenuRight( const InputEventPlus &input )
{
	MenuDown( input );
}

void ScreenNetEvaluation::MenuDown( const InputEventPlus &input )
{
	if ( m_iActivePlayers == 0 || !m_bHasStats )
		return;

	COMMAND( m_textUsers[m_iCurrentPlayer], "DeSel" );
	m_iCurrentPlayer = (m_iCurrentPlayer + 1) % m_iActivePlayers;
	COMMAND( m_textUsers[m_iCurrentPlayer], "Sel" );
	UpdateStats();
}

void ScreenNetEvaluation::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_GotEval)
	{
		m_bHasStats = true;

		LOG->Trace( "SMNETDebug:%d,%d",m_iActivePlayers,NSMAN->m_ActivePlayers );

		RedoUserTexts();

		LOG->Trace( "SMNETCheckpoint" );
		for( int i=0; i<m_iActivePlayers; ++i )
		{
			//Strange occourances because of timing
			//cause these things not to work right
			//and will sometimes cause a crash.
			//We should make SURE we won't crash!
			if ( size_t(i) >= NSMAN->m_EvalPlayerData.size() )
				break;

			if ( size_t(NSMAN->m_EvalPlayerData[i].name) >= NSMAN->m_PlayerNames.size() )
				break;

			if ( NSMAN->m_EvalPlayerData[i].name < 0 )
				break;

			if ( size_t(i) >= m_textUsers.size() )
				break;

			m_textUsers[i].SetText( NSMAN->m_PlayerNames[NSMAN->m_EvalPlayerData[i].name] );
			if ( NSMAN->m_EvalPlayerData[i].grade < Grade_Tier03 )	//Yes, hardcoded (I'd like to leave it that way)
				m_textUsers[i].SetRainbowScroll( true );
			else
				m_textUsers[i].SetRainbowScroll( false );
			ON_COMMAND( m_textUsers[i] );
			LOG->Trace( "SMNETCheckpoint%d", i );
		}
		return;	//no need to let ScreenEvaluation get ahold of this.
	}
	else if( SM == SM_GoToNextScreen )
	{
		NSMAN->ReportNSSOnOff( 4 );
	}
	ScreenEvaluation::HandleScreenMessage( SM );
}

void ScreenNetEvaluation::TweenOffScreen( )
{
	for( int i=0; i<m_iActivePlayers; ++i )	
		OFF_COMMAND( m_textUsers[i] );
	OFF_COMMAND( m_rectUsersBG );
	ScreenEvaluation::TweenOffScreen();
}

void ScreenNetEvaluation::UpdateStats()
{
	if( m_iCurrentPlayer >= (int) NSMAN->m_EvalPlayerData.size() )
		return;

	m_Grades[m_pActivePlayer].SetGrade( (Grade)NSMAN->m_EvalPlayerData[m_iCurrentPlayer].grade );

	m_textScore[m_pActivePlayer].SetText( ssprintf("%*.0i", NUM_SCORE_DIGITS, NSMAN->m_EvalPlayerData[m_iCurrentPlayer].score) );

	//Values greater than 6 will cause crash
	if ( NSMAN->m_EvalPlayerData[m_iCurrentPlayer].difficulty < 6 )
	{
		m_DifficultyIcon[m_pActivePlayer].SetPlayer( m_pActivePlayer );
		m_DifficultyIcon[m_pActivePlayer].SetFromDifficulty( NSMAN->m_EvalPlayerData[m_iCurrentPlayer].difficulty );
	}

	for( int j=0; j<NETNUMTAPSCORES; ++j )
	{
		int iNumDigits = (j==JudgeLine_MaxCombo)? MAX_COMBO_NUM_DIGITS:4;
		if( GAMESTATE->IsPlayerEnabled(m_pActivePlayer) ) // XXX: Why would this not be the case? -- Steve
			m_textJudgeNumbers[j][m_pActivePlayer].SetText( ssprintf("%*d", iNumDigits, NSMAN->m_EvalPlayerData[m_iCurrentPlayer].tapScores[j]) );
	}

	m_textPlayerOptions[m_pActivePlayer].SetText( NSMAN->m_EvalPlayerData[m_iCurrentPlayer].playerOptions );
}

#endif

/*
 * (c) 2004-2005 Charles Lohr, Joshua Allen
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

