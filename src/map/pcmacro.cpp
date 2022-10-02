// Copyright(C) 2022 Singe Horizontal
// Macro engine
// Version 1.0
// https://github.com/Singe-Horizontal
// This code is free software: you can redistribute it and/or modify 
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#include "pcmacro.hpp"
#include "pc.hpp"
namespace automatons {

Sequence& MacroCollection::get_current_macro_sequence() {
	return this->sequences[this->current_macro_id];
}
Sequence& MacroCollection::get_macro_sequence_at(int i){
	return this->sequences[i];
}
void MacroCollection::reset_all_macros(MacroCollection& macros){
	for (int i = 0; i < MAX_MACRO; ++i) {
		Sequence& sequence = macros.get_macro_sequence_at(i);
		sequence.reset();
	}
}
void Sequence::clear_all_steps(){
	this->reset();
	this->skill_reference_for_macro_range = 0;
	this->total_steps = 0;
	for (int i = 0; i < MAX_MACRO_STEPS; ++i) {
		Step& step = this->get_step(i);
		step.manipulated_object_id = 0;
		step.type = Step::Type::NONE;
		step.target = 0;
	}

}
bool MacroCollection::find_if_one_macro_is_active(MacroCollection& macros){
	for (int i = 0; i < MAX_MACRO; ++i) {
		Sequence& sequence = macros.get_macro_sequence_at(i);
			if(sequence.is_active())
				return true;
	};
	return false;
}

bool MacroCollection::skill_is_macro_starter(int skill_id) {
	return skill_id >= MACRO_START_ID && skill_id <= MACRO_END_ID;
}

void MacroCollection::set_current_macro_id(int id){
	this->current_macro_id = id;
}
void MacroCollection::set_sequence_for_macro(int id, Sequence& sequence){
	this->sequences[id]=sequence;
}


Step& Sequence::get_step(int i){
		return steps[i];
}

bool Sequence::is_active(){
	return (this->remaining_steps > 0 || this->tid != INVALID_TIMER);
}

void Sequence::set(Step& step,int i){
	steps[i]=step;
}

void Sequence::add_action_timer(t_tick skill_tick,t_tick equip_tick, int skill_id, int source_id) {
	Step& next_step = get_step(total_steps - remaining_steps);
	delay_before_next_skill = skill_tick;
	if (skill_id == WZ_JUPITEL)
		delay_before_next_skill += 150;
	else if (skill_id == CG_ARROWVULCAN || skill_id == AS_SONICBLOW)
		delay_before_next_skill = gettick() + 2000;
	else if (!next_step.is_animation_canceller())
		delay_before_next_skill = max(delay_before_next_skill, gettick() + automatons::Step::get_object_fixed_delay(skill_id));
	if (next_step.type == Step::Type::SKILL)
		tid = add_timer(delay_before_next_skill, macro_timer, source_id, 0);
	else if (next_step.type == Step::Type::EQUIPMENT)
		tid = add_timer(max(equip_tick, gettick() + 20), macro_timer, source_id, 0);
}

void Sequence::add_failed_skill_timer(t_tick casting_tick,t_tick equip_tick, int skill_id, int source_id) {
	Step& next_step = get_step(total_steps - remaining_steps);
	if (remaining_steps > 0) {
		this->delay_before_next_skill = casting_tick;
		if (skill_id == MG_SAFETYWALL) // special case
			tid = add_timer(gettick() + 100, macro_timer, source_id, 0);
		else
			tid = add_timer(delay_before_next_skill, macro_timer, source_id, 0);
	}

}


void Sequence::set_tick(t_tick new_tick){
	if(this->remaining_steps > 0)
		sett_tickimer(this->tid, new_tick);
}
void Sequence::reset(){
	this->tid = INVALID_TIMER;
	this->remaining_steps = 0;
	this->delay_before_next_skill = 0;
}
const std::vector<int> Step::fixed_delay_skills{ AL_HEAL,PR_STRECOVERY,PR_LEXDIVINA,
		PR_IMPOSITIO,HP_ASSUMPTIO,PR_LEXAETERNA,PR_SUFFRAGIUM,PR_ASPERSIO,
		AL_CURE,AM_POTIONPITCHER,SA_DISPELL,SL_KAUPE,SL_KAITE,PR_GLORIA,
		PR_SANCTUARY,PF_SPIDERWEB,CR_CULTIVATION,HW_GANBANTEIN,MG_SAFETYWALL,SA_LANDPROTECTOR,HW_MAGICPOWER,
		AC_SHOWER

	};
t_tick Step::get_object_fixed_delay(int id) {
	if (std::find(automatons::Step::fixed_delay_skills.begin(), automatons::Step::fixed_delay_skills.end(), id) != automatons::Step::fixed_delay_skills.end())
		return Step::animation_additional_delay;
	else
		return 0;
}

bool Step::is_animation_canceller() {
	if (type == Type::SKILL)
		return skill_get_inf(manipulated_object_id) == INF_SELF_SKILL; // No target skills cancel animation !
	else
		return false;
}

void Sequence::remove_fixed_delay_if_next_step_is_skill(Sequence& sequence, int source_id,t_tick ajusted_tick) {
	Step& next_step = sequence.get_step(sequence.total_steps - sequence.remaining_steps);
	if (sequence.is_active() && next_step.type == Step::Type::SKILL && ajusted_tick < Step::animation_additional_delay+gettick()) {
		sett_tickimer(sequence.tid, ajusted_tick);
	}
		
}
}
