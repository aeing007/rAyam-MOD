// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the body of custom script commands in this file.
 * Format:
 *	BUILDIN_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

//BUILDIN_FUNC(example)
//{
//	ShowInfo("buildin_example: It works!\n");
//	script_pushint(st,1);
//	return 0;
//}


/// Returns the skill's name [Easycore]
///
/// getskillname(<skill id>)
/// getskillname("<skill name>")
BUILDIN_FUNC(getskillname)
{
	uint16 skill_id = (script_isstring(st,2) ? skill_name2id(script_getstr(st,2)) : script_getnum(st,2));
	if (!skill_get_index(skill_id))
		script_pushconststr(st, "Unknown Skill");
	else
		script_pushstrcopy(st,skill_get_desc(skill_id));
	return SCRIPT_CMD_SUCCESS;
}
static bool autoparse(script_state* st, int total_parameters,std::vector<std::vector<int>>* v_autoparsed, bool ignore_empty_values = 0) {
	std::vector<char*> names(total_parameters);
	std::vector<int32> idxs(total_parameters);
	std::vector<int32> ids(total_parameters);
	std::vector<int64> values(total_parameters);
	std::vector<script_data*> script_datas(total_parameters);
	int k;
	for ( k = 0; k < total_parameters && script_hasdata(st, k + 2); k++) {
			;
		}
		if (k == total_parameters) {
			uint32 count  = script_getnum(st, 2+total_parameters);
			if (count > 0) {

				int j;
				for (j = 0; j < total_parameters; ++j) {

					if (script_hasdata(st, j + 2)) {
						script_datas[j] = script_getdata(st, j + 2);
						if (!data_isreference(script_datas[j])) {
							ShowError("script:auto: not an array\n");
							script_reportdata(script_datas[j]);
							st->state = END;
							return false;// not a variable
						}
						names[j] = reference_getname(script_datas[j]);
						idxs[j] = reference_getindex(script_datas[j]);
						ids[j] = reference_getid(script_datas[j]);
						if (is_string_variable(names[j])) {
							ShowError("script:auto: should be an array of numbers\n");
							script_reportdata(script_datas[j]);
							st->state = END;
							return false;
						}
					}
				}
				for (int i = 0; i < count; ++i) {
					if ((values[0] = get_val2_num(st, reference_uid(ids[0], idxs[0] + i), reference_getref(script_datas[0]))) <= 0)
						continue;
					for (j = 1; j < total_parameters &&
						((values[j]=get_val2_num(st, reference_uid(ids[j], idxs[j] + i), reference_getref(script_datas[j]))) > 0 || ignore_empty_values); ++j) {} //at that line it ignores nonexistent indexes since value at idxs+i== 0
					if (j == total_parameters) {
						std::vector<int> a_skill_data;
						for (j = 0; j < total_parameters; ++j) {
							a_skill_data.push_back(values[j]);
						}
						v_autoparsed->push_back(a_skill_data);
					}
				}
			}
		}
		return true;
}
static bool autoparse_ignore_empty(script_state* st, int total_parameters, std::vector<std::vector<int>>* v_autoparsed) {
	return autoparse(st, total_parameters, v_autoparsed, 1);
}

static void free_data_vector(intptr_t data) {
	std::vector<std::vector<int>>* v_autodata = (std::vector<std::vector<int>>*)data;
	delete v_autodata;
}
BUILDIN_FUNC(registermacro) {
	using namespace automatons;
	const unsigned int total_parameters = 2;

	TBL_PC* sd = NULL;
	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;// no player attached
	int current_macro_id=script_getnum(st,5) - 1;
	if (current_macro_id > MAX_MACRO)
		return SCRIPT_CMD_SUCCESS;

	std::vector<std::vector<int>> v_macro;
	if(!autoparse(st, total_parameters, &v_macro))
		return SCRIPT_CMD_FAILURE;
	Sequence& sequence = sd->macros.get_macro_sequence_at(current_macro_id);
	sequence.clear_all_steps();

	for (int i = 0; i < v_macro.size(); ++i) {
		int id = v_macro.at(i)[0];
		Step::Type type = static_cast<Step::Type>(v_macro.at(i)[1]);
		Step& step = sequence.get_step(i);
		step.type = type;
		step.manipulated_object_id = id;
		sequence.total_steps++;

		if (step.type == Step::Type::SKILL) {
			if (sequence.skill_reference_for_macro_range == 0) {
				sequence.skill_reference_for_macro_range = id;
				continue;
			}
			int possible_new_range = skill_get_range(step.manipulated_object_id, skill_get_max(step.manipulated_object_id));
			int range = skill_get_range(sequence.skill_reference_for_macro_range, skill_get_max(sequence.skill_reference_for_macro_range));
			if(possible_new_range > range)
				sequence.skill_reference_for_macro_range = step.manipulated_object_id;

		}
		sequence.set(step, i);
	}

	sd->macros.set_sequence_for_macro(current_macro_id, sequence);

	return SCRIPT_CMD_SUCCESS;
}

