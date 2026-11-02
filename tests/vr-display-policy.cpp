// SPDX-License-Identifier: GPL-3.0-or-later
// Engine-free regression tests of the actual display readiness/validation policy.
#include "VRDisplayPolicy.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
	unsigned checks{};
	void Check(bool value, const char* reason) { ++checks; if (!value) throw std::runtime_error(reason); }
}
int main()
{
	using namespace SKEE::VR::DisplayPolicy;
	try {
		const Request normal{30,0,100,100,0,1,0};
		{
			Readiness budget;
			Check(budget.Next(normal,1000)==Decision::attempt,"first readiness attempt refused");
			// A not-yet-ready scene can recover later under the same logical key.
			Check(budget.Next(normal,1250)==Decision::attempt,"same-key readiness retry lost");
			Check(budget.Attempts()==2,"readiness retries not counted");
			Check(budget.Next(normal,6999)==Decision::attempt,"deadline expired early");
			Check(budget.Next(normal,7000)==Decision::expired,"same request extended its deadline");
			Check(budget.Next(normal,7100)==Decision::expired,"expired request resumed");
		}
		{
			Readiness budget;
			Check(budget.Next(normal,0)==Decision::attempt,"zero clock origin rejected");
			for (std::uint64_t now=1;now<250;now+=7) Check(budget.Next(normal,now)==Decision::wait,"busy caller bypassed spacing");
			Check(budget.Attempts()==1,"throttled calls consumed attempts");
			Check(budget.Next(normal,250)==Decision::attempt,"spacing boundary rejected");
		}
		{
			Readiness budget;
			for (unsigned i=0;i<Readiness::maxAttempts;++i) Check(budget.Next(normal,i*250)==Decision::attempt,"finite attempt allowance shortened");
			Check(budget.Next(normal,Readiness::maxAttempts*250)==Decision::expired,"attempt cap bypassed");
			auto face=normal;face[6]=1;
			Check(budget.Next(face,Readiness::maxAttempts*250)==Decision::attempt,"real view transition did not start fresh request");
			Check(budget.Attempts()==1,"view transition inherited old attempt count");
		}
		{
			Readiness budget;
			Check(budget.Next(normal,200)==Decision::attempt,"clock guard precondition failed");
			Check(budget.Next(normal,199)==Decision::expired,"backwards clock admitted");
			Check(budget.Next(normal,450)==Decision::expired,"clock refusal not terminal for this key");
			Readiness reopened;
			Check(reopened.Next(normal,450)==Decision::attempt,"new session inherited terminal state");
		}
		{
			std::atomic<std::uint64_t> slot{2};
			ReleaseTicket(slot,1);
			Check(slot.load()==2,"old-session task cleared newer task ownership");
			ReleaseTicket(slot,2);
			Check(slot.load()==0,"own task ticket not released");
		}
		const auto nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
		for (float bad : {0.F,-1.F,nan,inf,-inf}) Check(!PositiveScale(bad),"invalid scale admitted");
		Check(PositiveScale(1.F),"ordinary scale refused");
		Check(OrthogonalAxes(55,55,0),"valid quad basis rejected");
		for (float dot : {0.02F,1.F,nan,inf}) Check(!OrthogonalAxes(55,55,dot),"skew/invalid quad basis admitted");
		Check(!OrthogonalAxes(0,55,0),"zero quad axis admitted");
		Check(!OrthogonalAxes(inf,55,0),"infinite quad axis admitted");
		Check(NormalizedYaw(158.2F)==158.2F,"default yaw changed");
		Check(NormalizedYaw(518.2F)>158.19F && NormalizedYaw(518.2F)<158.21F,"equivalent yaw not normalized");
		for (float bad : {nan,inf,-inf}) Check(NormalizedYaw(bad)==180.F,"invalid yaw fallback lost");
		Check(std::isfinite(NormalizedYaw(std::numeric_limits<float>::max())),"huge finite yaw overflowed");
		{
			LogBudget budget;
			for (unsigned i=0;i<LogBudget::limit;++i) Check(budget.Take(),"normal observation allowance shortened");
			for (unsigned i=0;i<40;++i) Check(!budget.Take(),"diagnostic flood limit bypassed");
			Check(budget.Suppressed()==40,"suppressed diagnostics not accounted");
		}
		std::cout<<checks<<" VR display policy checks passed\n";
		return 0;
	} catch (const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
