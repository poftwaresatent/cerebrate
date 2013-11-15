/* 
 * Copyright (C) 2005 Roland Philippsen <roland dot philippsen at gmx dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */


#include "Cactus.hpp"
#include "MotionManager.hpp"
#include "Odometry.hpp"
#include "Scanalyzer.hpp"
#include "Localizer.hpp"
#include "CircleLSQ.hpp"
#include "Behavior.hpp"
#include "Watchdog.hpp"
#include "Effects.hpp"
#include <gfx/wrap_gl.hpp>
#include <gfx/Mousehandler.hpp>
#include <drivers/FModIPDCMOT.hpp>
#include <drivers/FModTCP.hpp>
#include <sfl/numeric.hpp>
#include <iostream>


using namespace std;
using namespace sfl;


Cactus::
Cactus():
  _state(MANUAL_SPEED),
  _enable_auto_localize(false)
{
  _left = auto_ptr<FModIPDCMOT>(FModIPDCMOT::Create(8010, "rinks",
						    100000, // usec_cycle
						    1.5, // kp
						    0.01, // ki
						    0.0, // kd
						    100000, // tspeed
						    2.0, // imax
						    10000)); // acc
  if(_left.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _left.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  _right = auto_ptr<FModIPDCMOT>(FModIPDCMOT::Create(8010, "lechts",
						     100000,
						     1.5,
						     0.01,
						     0.0,
						     100000,
						     2.0,
						     10000));
  if(_right.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _right.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  _io = auto_ptr<FModTCP>(FModTCP::Create(8010, "iocactus",
					  10000000,	// usec_cycle
					  3));	// max_errcount
  if(_io.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _io.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  _odometry = auto_ptr<Odometry>(new Odometry(* _left, * _right,
					      WHEELBASE, WHEELRADIUS));
  if(_odometry.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _odometry.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  static const double sdmax(0.4);
  _motion_manager = auto_ptr<MotionManager>(new MotionManager(* _odometry,
							      * _left,
							      * _right,
							      sdmax));
  if(_motion_manager.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _motion_manager.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  static const double cluster_thresh(0.05);
  static const double rhomax(8);
  static const string comport("/dev/tts/0");
  static const unsigned long baudrate(38400L);
  static const unsigned int usec_cycle(100000);
  _scanalyzer = auto_ptr<Scanalyzer>(new Scanalyzer(cluster_thresh,
						    rhomax,
						    "bgmap",
						    comport,
						    baudrate,
						    usec_cycle));
  if(_scanalyzer.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _scanalyzer.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  _localizer = auto_ptr<Localizer>(new Localizer(* _odometry,
						 * _motion_manager,
						 * _scanalyzer));
  if(_localizer.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _localizer.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  static const double home_x(1);
  static const double home_y(2.5);
  static const double green(4.2);
  static const double red(2.5);
  _behavior = auto_ptr<Behavior>(new Behavior(home_x, home_y, green, red));
  if(_behavior.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _behavior.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  static const Timestamp loc_tout(30);
  static const Timestamp sick_ok_tout(1);
  static const Timestamp sick_recover_tout(10);
  _watchdog = auto_ptr<Watchdog>(new Watchdog(* _localizer,
					      loc_tout,
					      * _scanalyzer,
					      sick_ok_tout,    
					      sick_recover_tout,    
					      * _left,
					      * _right));
  if(_watchdog.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _watchdog.get() == 0\n";
    exit(EXIT_FAILURE);
  }
  
  _effects = auto_ptr<Effects>(new Effects( * _io));
  if(_effects.get() == 0){
    cerr << "FATAL ERROR in Cactus::Cactus(): _effects.get() == 0\n";
    exit(EXIT_FAILURE);
  }
}


Cactus::
~Cactus()
{
  SetQd(0, 0);
}


void Cactus::
SetQd(double qdl, double qdr)
{
  _state = MANUAL_SPEED;
  _motion_manager->SetQd(qdl, qdr);
}


bool Cactus::
_SetShoulder(bool on, std::ostream * dbg)
{
  return _effects->_SetShoulder(on, dbg);
}


bool Cactus::
_SetEar(bool on, std::ostream * dbg)
{
  return _effects->_SetEar(on, dbg);
}


bool Cactus::
_SetLight(int num, bool on, std::ostream * dbg)
{
  return _effects->_SetLight(num, on, dbg);
}


bool Cactus::
Command(std::istream & is, std::ostream & os, std::ostream * dbg)
{
  double anchor_x(0), anchor_y(0);
  string cmd;
  if( ! (is >> cmd))
    os << "ERROR reading command.\n";
  else if((cmd == "h") || (cmd == "help"))
    os << "**************************************************\n"
       << "* CACTUS CONSOLE COMMANDS\n"
       << "**************************************************\n"
       << " b (brake)               Emergency brake.\n"
       << " h (help)                Show this message.\n"
       << " q (quit)                End program.\n"
       << " sq       <qdl> <qdr>    Set actuator speed.\n"
       << " sg       <sd> <thetad>  Set global speed.\n"
       << " son                     Switch shoulder motor on.\n"
       << " soff                    Switch shoulder motor off.\n"
       << " eon                     Switch ear motor on.\n"
       << " eoff                    Switch ear motor off.\n"
       << " lon      <num>          Switch on light #num.\n"
       << " loff     <num>          Switch off light #num.\n"
       << " sscan    <filename>     Save current scan to file.\n"
       << " sbg      <filename>     Save current background map to file.\n"
       << " lbg      <filename>     Load background map from file.\n"
       << " ixy                     Init (x, y).\n"
       << " itheta                  Init theta (only after ixy!!).\n"
       << " loc                     Localization heuristic.\n"
       << " go      <x> <y> <theta> Go to goal.\n"
       << " gpref   <dr> <dtheta>   Set goal precision.\n"
       << " pose                    Show pose.\n"
       << " spose   <x> <y> <theta> Set pose.\n"
       << " start                   Start using behavior.\n"
       << " stop                    Stop using behavior.\n"
       << " alon                    Enable automatic (re)localizing.\n"
       << " aloff                   Disable automatic (re)localizing.\n";
  
  else if((cmd == "b") || (cmd == "brake"))
    _motion_manager->Wait();
  else if((cmd == "q") || (cmd == "quit")){
    os << "Byebye!\n";
    return false;
  }
  else if(cmd == "sq"){
    double qdl, qdr;
    if( ! (is >> qdl))
      os << "ERROR reading qdl.\n";
    else if( ! (is >> qdr))
      os << "ERROR reading qdr.\n";
    SetQd(qdl, qdr);
  }
  else if(cmd == "sg"){
    double sd, thetad;
    if( ! (is >> sd))
      os << "ERROR reading sd.\n";
    else if( ! (is >> thetad))
      os << "ERROR reading thetad.\n";
    SetSpeed(sd, thetad);
  }
  else if(cmd == "son"){
    if( ! _SetShoulder(true, dbg))
      os << "ERROR in SetShoulder(true).\n";
  }
  else if(cmd == "soff"){
    if( ! _SetShoulder(false, dbg))
      os << "ERROR in SetShoulder(false).\n";
  }
  else if(cmd == "eon"){
    if( ! _SetEar(true, dbg))
      os << "ERROR in SetEar(true).\n";
  }
  else if(cmd == "eoff"){
    if( ! _SetEar(false, dbg))
      os << "ERROR in SetEar(false).\n";
  }
  else if(cmd == "lon"){
    int num;
    if( ! (is >> num))
      os << "ERROR reading num.\n";
    else if( ! _SetLight(num, true, dbg))
      os << "ERROR in SetLight(" << num << ", true).\n";
  }
  else if(cmd == "loff"){
    int num;
    if( ! (is >> num))
      os << "ERROR reading num.\n";
    else if( ! _SetLight(num, false, dbg))
      os << "ERROR in SetLight(" << num << ", false).\n";
  }
  else if(cmd == "sscan"){
    string fname;
    if( ! (is >> fname))
      os << "ERROR reading filename.\n";
    else if( ! _localizer->SaveScan(fname))
      os << "ERROR in SaveScan(" << fname << ").\n";
  }
  else if(cmd == "sbg"){
    string fname;
    if( ! (is >> fname))
      os << "ERROR reading filename.\n";
    else if( ! _localizer->SaveBackground(fname))
      os << "ERROR in SaveBackground(" << fname << ").\n";
  }
  else if(cmd == "lbg"){
    string fname;
    if( ! (is >> fname))
      os << "ERROR reading filename.\n";
    else if( ! _localizer->LoadBackground(fname))
      os << "ERROR in LoadBackground(" << fname << ").\n";
  }
  else if(cmd == "ixy"){
    if( ! InitOdometryXY(anchor_x, anchor_y, dbg))
      os << "ERROR in InitOdometryXY().\n";
  }
  else if(cmd == "itheta"){
    if( ! InitOdometryTheta(anchor_x, anchor_y, dbg))
      os << "ERROR in InitOdometryTheta().\n";
  }
  else if(cmd == "go"){
    double gx, gy, gtheta;
    if( ! (is >> gx >> gy >> gtheta))
      os << "ERROR reading goal.\n";
    SetGoal(gx, gy, gtheta);
  }
  else if(cmd == "gpref"){
    double dr, dtheta;
    if( ! (is >> dr >> dtheta))
      os << "ERROR reading goal parameters.\n";
    SetGoalPrecision(dr, dtheta);
  }
  else if(cmd == "pose"){
    const Frame & pose(_odometry->GetCurrentPose());
    os << "pose = (" << pose.X() << ", " << pose.Y() << ", "
       << pose.Theta() << ")\n";
  }
  else if(cmd == "loc"){
    if( ! AutoLocalize(dbg))
      os << "ERROR in AutoLocalize().\n";
  }
  else if(cmd == "spose"){
    double x, y, theta;
    if( ! (is >> x >> y >> theta))
      os << "ERROR reading pose.\n";
    _odometry->Init(Timestamp::Now(), x, y, theta, _motion_manager.get());
  }
  else if(cmd == "start"){
    SetState(AUTO);
  }
  else if(cmd == "stop"){
    SetState(MANUAL_SPEED);
  }
  else if(cmd == "alon"){
    SetEnableAutoLocalize(true);
  }
  else if(cmd == "aloff"){
    SetEnableAutoLocalize(false);
  }
  else
    os << "ERROR: unknown command \"" << cmd << "\"\n";
  return true;
}


void Cactus::
Draw() const
{
  _behavior->DrawRegions();
  _localizer->DrawPrediction(_odometry->GetCurrentPose());
  _odometry->Draw();
  _motion_manager->Draw();
  _behavior->DrawTarget();
  _localizer->Draw();
}


bool Cactus::
InitOdometryXY(double & anchor_x, double & anchor_y, ostream * dbg)
{
  const CircleLSQ * best(_localizer->FindBest());
  if(0 == best){
    if(0 != dbg)
      (*dbg) << "ERROR in Cactus::InitOdometryXY():"
	     << " No good circle found.\n";
    return false;
  }
  
  anchor_x = best->xc;
  anchor_y = best->yc;
  return true;
}


bool Cactus::
InitOdometryTheta(double anchor_x, double anchor_y, std::ostream * dbg)
{
  const CircleLSQ * best(_localizer->FindBest());
  if(0 == best){
    if(0 != dbg)
      (*dbg) << "ERROR in Cactus::InitOdometryTheta():"
	     << " No good circle found.\n";
    return false;
  }

  static const bool sanity_check(false);
  if(sanity_check){
    // Check if the distance to anchor corresponds roughly to the
    // distance odometry has travelled since (which is in motion
    // manager's anchor).
    const Frame pose(_odometry->GetCurrentPose());
    const Frame & mm_anchor(_motion_manager->GetAnchor().GetPose());
    const double odx(pose.X() - mm_anchor.X());
    const double ody(pose.Y() - mm_anchor.Y());
    const double ods(sqrt(sqr(odx) + sqr(ody)));
    const double cdx(best->xc - anchor_x);
    const double cdy(best->yc - anchor_y);
    const double cds(sqrt(sqr(cdx) + sqr(cdy)));
    static const double foo_thresh(0.05);
    if(absval(cds - ods) > foo_thresh){
      if(0 != dbg)
	(*dbg) << "ERROR in Cactus::InitOdometryTheta():\n"
	       << "  Motion manager says we travelled ods = " << ods << "\n"
	       << "  But the two matched circles are cds =  " << cds
	       << " apart.\n";
      return false;
    }
    _odometry->Init(Timestamp::Now(), best->xc, best->yc, atan2(cdy, cdx),
		    _motion_manager.get());
  }
  else  
    _odometry->Init(Timestamp::Now(), best->xc, best->yc,
		    atan2(best->yc - anchor_y, best->xc - anchor_x),
		    _motion_manager.get());
  
  if(0 != dbg)
    (*dbg) << "DBG Cactus::InitOdometryTheta():\n"
	   << "  pose       = " << best->xc << ", " << best->yc
	   << ", " << atan2(best->yc - anchor_y, best->xc - anchor_x) << "\n"
	   << "  crosscheck = " << _odometry->GetCurrentPose().X() << ", "
	   << _odometry->GetCurrentPose().Y() << ", "
	   << _odometry->GetCurrentPose().Theta() << "\n";
  
  return true;
}


void Cactus::
SetGoal(double x, double y, double theta)
{
  _state = MANUAL_GOAL;
  _motion_manager->SetGoal(x, y, theta);
}


void Cactus::
SetGoalPrecision(double dr, double dtheta)
{
  _state = MANUAL_GOAL;
  _motion_manager->SetGoalPrecision(dr, dtheta);
}


void Cactus::
SetSpeed(double sd, double thetad)
{
  _state = MANUAL_SPEED;
  _motion_manager->SetSpeed(sd, thetad);
}


bool Cactus::
AutoLocalize(std::ostream * dbg)
{
  state_t old_state(_state);
  
  SetQd(-1, -1);
  usleep(1200000);
  SetQd(0, 0);
  usleep(1500000);
  
  double anchor_x, anchor_y;  
  if( ! InitOdometryXY(anchor_x, anchor_y, dbg)){
    if(0 != dbg)
      (*dbg) << "ERROR in InitOdometryXY().\n";
    SetQd(1, 1);
    usleep(1200000);
    SetQd(0, 0);
    _state = old_state;
    return false;
  }
  
  SetQd(1, 1);
  usleep(1200000);
  SetQd(0, 0);
  usleep(1500000);
  
  if( ! InitOdometryTheta(anchor_x, anchor_y, dbg)){
    if(0 != dbg)
      (*dbg) << "ERROR in InitOdometryTheta().\n";
    _state = old_state;
    return false;
  }
 
  if(0 != dbg){
    const Frame & pose(_odometry->GetCurrentPose());
    (*dbg) << "pose = (" << pose.X() << ", " << pose.Y() << ", "
	   << pose.Theta() << ")\n";
  }
  _state = old_state;
  return true;
}


/** \todo scanalyzer is updated through localizer, same for drawing... */
void Cactus::
Update()
{
  _odometry->Update();
  _localizer->Update();
  static const double deadzone(0.8);
  const Frame & pose(_odometry->GetCurrentPose());
  
  switch(_state){
  case MANUAL_SPEED:
  case MANUAL_GOAL:
    _behavior->Update(_localizer->GetScanalysis(), pose, deadzone);
    _effects->SetLightshow(5, 6,
			   5, 6,
			   5, 6,
			   5, 6,
			   5, 6);
    break;
  case MANUAL_TARGET:
    _behavior->FakeUpdate(_fake_target_x, _fake_target_y);
    _behavior->Perform(pose, * _motion_manager, * _effects);
    break;
  case AUTO:
    _behavior->Update(_localizer->GetScanalysis(), pose, deadzone);
    _behavior->Perform(pose, * _motion_manager, * _effects);
    break;
  default:
    cerr << "ERROR in Cactus::Update(): Illegal state " << _state << "\n";
    exit(EXIT_FAILURE);
  }
  _effects->UpdateLightshow();
  _motion_manager->Update();

  UpdateWatchdog();
}


void Cactus::
UpdateWatchdog()
{
  _watchdog->Update();
  switch(_watchdog->GetSickState()){
  case Watchdog::ILL:
    cerr << ":o( SICK is ILL!\n";
    _watchdog->RestartSick();
    break;
  case Watchdog::OK:
  case Watchdog::RECOVERING:
    // wait...
    break;
  case Watchdog::DEAD:
    cerr << ":o( SICK is DEAD!\n";
    _motion_manager->Wait();
    exit(EXIT_FAILURE);
  default:
    cerr << "ERROR in Cactus::Update(): illegal sick state "
	 << _watchdog->GetSickState() << "\n";
    exit(EXIT_FAILURE);
  }
  
  if(_enable_auto_localize && (Watchdog::OK != _watchdog->GetLocalizerState()))
    AutoLocalize(0);
  
  //   if( ! _watchdog->MotorsOk()){
  //     cerr << ":o( motors blocked!\n";
  //     //    _motion_manager->UnblockMotors();
  //   }
}


void Cactus::
GetPose(double & x, double & y, double & theta) const
{
  const Frame & pose(_odometry->GetCurrentPose());
  x = pose.X();
  y = pose.Y();
  theta = pose.Theta();
}


class CMhandler: public Mousehandler {
public:
  typedef enum { GOAL, TARGET, AUTO } variant_t;
  CMhandler(Cactus * that, variant_t variant):
    _that(that), _variant(variant) {}
  virtual void HandleClick(double x, double y) {
    if(GOAL == _variant)
      _that->SetGoal(x, y, - M_PI/2);
    else if(TARGET == _variant)
      _that->SetTarget(x, y);
    else
      _that->SetState(Cactus::AUTO);
  }
  Cactus * _that;
  const variant_t _variant;
};


Mousehandler * Cactus::
GetMouseGoal()
{
  if(_mousegoal.get() == 0)
    _mousegoal =
      auto_ptr<Mousehandler>(new CMhandler(this, CMhandler::GOAL));
  return _mousegoal.get();
}


Mousehandler * Cactus::
GetMouseTarget()
{
  if(_mousetarget.get() == 0)
    _mousetarget =
      auto_ptr<Mousehandler>(new CMhandler(this, CMhandler::TARGET));
  return _mousetarget.get();
}


Mousehandler * Cactus::
GetMouseAuto()
{
  if(_mouseauto.get() == 0)
    _mouseauto =
      auto_ptr<Mousehandler>(new CMhandler(this, CMhandler::AUTO));
  return _mouseauto.get();
}


void Cactus::
DrawStatus() const
{
  _watchdog->Draw();
}


void Cactus::
DrawEffects() const
{
  _effects->Draw();
}


void Cactus::
ConfigureStatusViewport(Viewport & vp) const
{
  _watchdog->ConfigureViewport(vp);
}


void Cactus::
ConfigureEffectsViewport(Viewport & vp) const
{
  _effects->ConfigureViewport(vp);
}


void Cactus::
SetState(state_t state)
{
  _state = state;
}


void Cactus::
SetEnableAutoLocalize(bool enable)
{
  _enable_auto_localize = enable;
}


void Cactus::
SetTarget(double x, double y)
{
  _fake_target_x = x;
  _fake_target_y = y;
  _state = MANUAL_TARGET;
}


void Cactus::
ConfigureArenaViewport(Viewport & vp) const
{
  _scanalyzer->ConfigureViewport(vp);
}
