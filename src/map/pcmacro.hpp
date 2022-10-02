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

#ifndef PCMACRO_HPP
#define PCMACRO_HPP
#include <vector>
#include "../common/timer.hpp"
#include <array>

#define MAX_MACRO 12 /// Max macro
#define MAX_MACRO_STEPS 25 /// Max macro steps
namespace automatons {
class MacroCollection;
class Sequence;
class Step;



class Step {
public:

	enum class Type {
		NONE = 0,
		SKILL = 1,
		EQUIPMENT = 2
	};

	int manipulated_object_id = 0;
	Type type = Type::NONE;
	unsigned int target = 0;

	
	static const std::vector<int> fixed_delay_skills;
	static const t_tick animation_additional_delay = 625;

	static t_tick get_object_fixed_delay(int id);

	bool is_animation_canceller();

	Step() = default;
};

class Sequence {
private:
	std::array<Step,MAX_MACRO_STEPS> steps;	
public:
	int total_steps;
	int skill_reference_for_macro_range = 0;

	int tid = INVALID_TIMER;
	int remaining_steps = 0;
	t_tick delay_before_next_skill = 0;

	Sequence() = default;

	Step& get_step(int i);

	void set(Step& step, int i);
	void add_action_timer(t_tick scheduled_casting_tick, t_tick equip_tick, int skill_id, int source_id);
	void add_failed_skill_timer(t_tick scheduled_casting_tick, t_tick equip_tick, int skill_id, int source_id);
	void set_tick(t_tick new_tick);
	static void remove_fixed_delay_if_next_step_is_skill(Sequence& sequence,int source_id,t_tick restored_tick );

	bool is_active();
	void reset();
	void clear_all_steps();
};
class MacroCollection {
public:
	std::array<Sequence,MAX_MACRO> sequences;

	Sequence& get_current_macro_sequence();
	Sequence& get_macro_sequence_at(int i);

	static void reset_all_macros(MacroCollection& macros);
	static bool find_if_one_macro_is_active(MacroCollection& macros);
	static bool skill_is_macro_starter(int skill_id);

	void set_current_macro_id(int id);
	void set_sequence_for_macro(int id,Sequence& sequence);

	MacroCollection() = default;

private:
	int current_macro_id = 0;
};

}
#endif
