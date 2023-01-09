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
//Same function without error messages to not flood the chat
static bool skill_check_condition_castbegin_nocliffail(struct map_session_data* sd, uint16 skill_id, uint16 skill_lv)
{
	struct status_data *status;
	struct status_change *sc;
	struct s_skill_condition require;
	int i;

	if (sd->chatID)
		return false;

	if( pc_has_permission(sd, PC_PERM_SKILL_UNCONDITIONAL) && sd->skillitem != skill_id )
	{	//GMs don't override the skillItem check, otherwise they can use items without them being consumed! [Skotlex]
		sd->state.arrow_atk = skill_get_ammotype(skill_id)?1:0; //Need to do arrow state check.
		sd->spiritball_old = sd->spiritball; //Need to do Spiritball check.
		sd->soulball_old = sd->soulball; //Need to do Soulball check.
		sd->servantball_old = sd->servantball; //Need to do Servantball check.
		sd->abyssball_old = sd->abyssball; //Need to do Abyssball check.
		return true;
	}

	status = &sd->battle_status;
	sc = &sd->sc;
	if( !sc->count )
		sc = NULL;
	if( pc_is90overweight(sd) ) {
		return false;
	}
	//Checks if disabling skill - in which case no SP requirements are necessary
	if( sc && skill_disable_check(*sc,skill_id))
		return true;

	std::bitset<INF2_MAX> inf2 = skill_db.find(skill_id)->inf2;

	// Check the skills that can be used while mounted on a warg
	if( pc_isridingwug(sd) ) {
		if(!inf2[INF2_ALLOWONWARG])
			return false; // in official there is no message.
	}
	require = skill_get_requirement(sd,skill_id,skill_lv);
	//Can only update state when weapon/arrow info is checked.
	sd->state.arrow_atk = require.ammo?1:0;

	// perform skill-specific checks (and actions)
	switch( skill_id ) {
		case AS_CLOAKING:
		{
			if( skill_lv < 3 && ((sd->bl.type == BL_PC && battle_config.pc_cloak_check_type&1)
			||	(sd->bl.type != BL_PC && battle_config.monster_cloak_check_type&1) )) { //Check for walls.
				static int dx[] = { 0, 1, 0, -1, -1,  1, 1, -1};
				static int dy[] = {-1, 0, 1,  0, -1, -1, 1,  1};
				int di;
				ARR_FIND( 0, 8, di, map_getcell(sd->bl.m, sd->bl.x+dx[di], sd->bl.y+dy[di], CELL_CHKNOPASS) != 0 );
				if( di == 8 ) {
					return false;
				}
			}
			break;
		}
		case MO_CALLSPIRITS:
			if(sc && sc->getSCE(SC_RAISINGDRAGON))
				skill_lv += sc->getSCE(SC_RAISINGDRAGON)->val1;
			if(sd->spiritball >= skill_lv) {
				return false;
			}
			break;
		case ASC_EDP:
#ifdef RENEWAL
			if (sd->weapontype1 == W_FIST && battle_config.switch_remove_edp&2) {
#else
			if (sd->weapontype1 == W_FIST && battle_config.switch_remove_edp&1) {
#endif
				return false;
			}
			break;
		case TK_READYCOUNTER:
		case TK_READYDOWN:
		case TK_READYSTORM:
		case TK_READYTURN:
		case TK_JUMPKICK:
			if( (sd->class_&MAPID_UPPERMASK) == MAPID_SOUL_LINKER ) { // Soul Linkers cannot use this skill
				return false;
			}
			break;
		case PR_BENEDICTIO:
			if (skill_check_pc_partner(sd, skill_id, &skill_lv, 1, 0) < 2) {
				return false;
			}
			break;
		case SG_SUN_WARM:
		case SG_MOON_WARM:
		case SG_STAR_WARM:
			if (sc && sc->getSCE(SC_MIRACLE))
				break;
			i = skill_id-SG_SUN_WARM;
			if (sd->bl.m == sd->feel_map[i].m)
				break;
			return false;
			break;
		case SG_SUN_COMFORT:
		case SG_MOON_COMFORT:
		case SG_STAR_COMFORT:
			if (sc && sc->getSCE(SC_MIRACLE))
				break;
			i = skill_id-SG_SUN_COMFORT;
			if (sd->bl.m == sd->feel_map[i].m &&
				(battle_config.allow_skill_without_day || sg_info[i].day_func()))
				break;
			return false;
		case SG_FUSION:
			if (sc && sc->getSCE(SC_SPIRIT) && sc->getSCE(SC_SPIRIT)->val2 == SL_STAR)
				break;
			//Auron insists we should implement SP consumption when you are not Soul Linked. [Skotlex]
			//Only invoke on skill begin cast (instant cast skill). [Kevin]
			if( require.sp > 0 ) {
				if (status->sp < (unsigned int)require.sp)
					;
				else
					status_zap(&sd->bl, 0, require.sp);
			}
			return false;
		case GS_GLITTERING:
		case RL_RICHS_COIN:
			if(sd->spiritball >= 10) {
				return false;
			}
			break;
		case NJ_ISSEN:
#ifdef RENEWAL
			if (status->hp < (status->hp/100)) {
#else
			if (status->hp < 2) {
#endif
				return false;
			}
		case NJ_BUNSINJYUTSU:
			if (!(sc && sc->getSCE(SC_NEN))) {
				clif_skill_fail(sd,skill_id,USESKILL_FAIL_LEVEL,0);
				return false;
			}
			break;
		case PF_HPCONVERSION:
			if (status->sp == status->max_sp)
				return false; //Unusable when at full SP.
			break;
		case SP_KAUTE: // Fail if below 30% MaxHP.
			if (status->hp < 30 * status->max_hp / 100) {
				return false;
			}
			break;
		case BS_ADRENALINE:
		case BS_ADRENALINE2:
		case KN_TWOHANDQUICKEN:
		case KN_ONEHAND:
		case CR_SPEARQUICKEN:
		case WS_CARTBOOST:
			if (sc->getSCE(SC_QUAGMIRE) || sc->getSCE(SC_DONTFORGETME))
				return false;
			break;
		case AC_CONCENTRATION:
			if (sc->getSCE(SC_QUAGMIRE))
				return false;
			break;
	}

	/* check state required */
	switch (require.state) {
		case ST_HIDDEN:
			if(!pc_ishiding(sd)) {
				return false;
			}
			break;
		case ST_RIDING:
			if(!pc_isriding(sd) && !pc_isridingdragon(sd)) {
				return false;
			}
			break;
		case ST_FALCON:
			if(!pc_isfalcon(sd)) {
				return false;
			}
			break;
		case ST_CART:
			if(!pc_iscarton(sd)) {
				return false;
			}
			break;
		case ST_SHIELD:
			if(sd->status.shield <= 0) {
				return false;
			}
			break;
		case ST_RECOVER_WEIGHT_RATE:
#ifdef RENEWAL
			if(pc_is70overweight(sd)) {
#else
			if(pc_is50overweight(sd)) {
#endif
				return false;
			}
			break;
		case ST_MOVE_ENABLE:
			if (sc && sc->getSCE(SC_COMBO) && sc->getSCE(SC_COMBO)->val1 == skill_id)
				sd->ud.canmove_tick = gettick(); //When using a combo, cancel the can't move delay to enable the skill. [Skotlex]

			if (!unit_can_move(&sd->bl)) {
				return false;
			}
			break;
		case ST_WATER:
			if (sc && (sc->getSCE(SC_DELUGE) || sc->getSCE(SC_SUITON)))
				break;
			if (map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKWATER) && !map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKLANDPROTECTOR))
				break;
			return false;
		case ST_RIDINGDRAGON:
			if( !pc_isridingdragon(sd) ) {
				return false;
			}
			break;
		case ST_WUG:
			if( !pc_iswug(sd) ) {
				return false;
			}
			break;
		case ST_RIDINGWUG:
			if( !pc_isridingwug(sd) ) {
				return false;
			}
			break;
		case ST_MADO:
			if( !pc_ismadogear(sd) ) {
				return false;
			}
			break;
		case ST_ELEMENTALSPIRIT:
		case ST_ELEMENTALSPIRIT2:
			if(!sd->ed) {
				return false;
			}
			break;
		case ST_PECO:
			if(!pc_isriding(sd)) {
				return false;
			}
			break;
		case ST_SUNSTANCE:
			if (!(sc && (sc->getSCE(SC_SUNSTANCE) || sc->getSCE(SC_UNIVERSESTANCE)))) {
				return false;
			}
			break;
		case ST_MOONSTANCE:
			if (!(sc && (sc->getSCE(SC_LUNARSTANCE) || sc->getSCE(SC_UNIVERSESTANCE)))) {
				return false;
			}
			break;
		case ST_STARSTANCE:
			if (!(sc && (sc->getSCE(SC_STARSTANCE) || sc->getSCE(SC_UNIVERSESTANCE)))) {
				return false;
			}
			break;
		case ST_UNIVERSESTANCE:
			if (!(sc && sc->getSCE(SC_UNIVERSESTANCE))) {
				return false;
			}
			break;
	}
	// Check for equipped item(s)
	if (!require.eqItem.empty()) {
		size_t count = require.eqItem.size();

		for (const auto &it : require.eqItem) {
			t_itemid reqeqit = it;

			if (!reqeqit)
				break; // Skill has no required item(s); get out of here
			switch(skill_id) { // Specific skills require multiple items while default will handle singular cases
				case NC_PILEBUNKER:
				case RL_P_ALTER:
					if (!pc_checkequip2(sd,reqeqit,EQI_ACC_L,EQI_MAX)) {
						count--;
						if (!count) {
							if( skill_id == RL_P_ALTER ){
								clif_msg( sd, SKILL_NEED_HOLY_BULLET );
							}else{
								clif_skill_fail(sd,skill_id,USESKILL_FAIL_THIS_WEAPON,0);
							}
							return false;
						} else
							continue;
					}
					break;
				case NC_ACCELERATION:
				case NC_SELFDESTRUCTION:
				case NC_SHAPESHIFT:
				case NC_EMERGENCYCOOL:
				case NC_MAGNETICFIELD:
				case NC_NEUTRALBARRIER:
				case NC_STEALTHFIELD:
					if (pc_search_inventory(sd, reqeqit) == -1) {
						count--;
						if (!count) {
							clif_skill_fail(sd, skill_id, USESKILL_FAIL_NEED_EQUIPMENT, 0, require.eqItem[0]);
							return false;
						} else
							continue;
					}
					break;
				default:
					if (!pc_checkequip2(sd,reqeqit,EQI_ACC_L,EQI_MAX)) {
						clif_skill_fail(sd, skill_id, USESKILL_FAIL_NEED_EQUIPMENT, 0, reqeqit);
						return false;
					}
					break;
			}
		}
	}

	if(require.mhp > 0 && get_percentage(status->hp, status->max_hp) > require.mhp) {
		//mhp is the max-hp-requirement, that is,
		//you must have this % or less of HP to cast it.
		return false;
	}

	if( require.weapon && !pc_check_weapontype(sd,require.weapon) ) {
		switch(skill_id) {
			case RA_AIMEDBOLT:
				break;
			default:
				switch((unsigned int)log2(require.weapon)) {
					case W_REVOLVER:
						break;
					case W_RIFLE:
						break;
					case W_GATLING:
						break;
					case W_SHOTGUN:
						break;
					case W_GRENADE:
						break;
					default:
						break;
				}
				return false;
		}
	}

	if( require.sp > 0 && status->sp < (unsigned int)require.sp) {
		return false;
	}

	if (require.ap > 0 && status->ap < (unsigned int)require.ap) {
		return false;
	}

	if( require.zeny > 0 && sd->status.zeny < require.zeny ) {
		return false;
	}

	if (require.spiritball > 0) { // Skills that require certain types of spheres to use.
		switch (skill_id) { // Skills that require soul spheres.
			case SP_SOULGOLEM:
			case SP_SOULSHADOW:
			case SP_SOULFALCON:
			case SP_SOULFAIRY:
			case SP_SOULCURSE:
			case SP_SPA:
			case SP_SHA:
			case SP_SWHOO:
			case SP_SOULUNITY:
			case SP_SOULDIVISION:
			case SP_SOULREAPER:
			case SP_SOULEXPLOSION:
			case SP_KAUTE:
				if (sd->soulball < require.spiritball) {
					return false;
				}
				break;

			// Skills that requires servants.
			case DK_SERVANT_W_SIGN:
			case DK_SERVANT_W_PHANTOM:
			case DK_SERVANT_W_DEMOL:
				if (sd->servantball < require.spiritball) {
					return false;
				}
				break;

			default: // Skills that require spirit/coin spheres.
				if (sd->spiritball < require.spiritball) {
					return false;
				}
				break;
		}
	}

	return true;
}

static bool parse_skill_toid( struct map_session_data* sd, uint16 skill_id, uint16 skill_lv, int target_id ){
	t_tick tick = gettick();

	if( skill_lv < 1 ) skill_lv = 1; //No clue, I have seen the client do this with guild skills :/ [Skotlex]

	int inf = skill_get_inf(skill_id);
	if (inf&INF_GROUND_SKILL || !inf)
		return false; //Using a ground/passive skill on a target? WRONG.

	if (sd->state.block_action & PCBLOCK_SKILL) {
		clif_msg(sd, WORK_IN_PROGRESS);
		return false;
	}

	if( sd->npc_id ){
		if( pc_hasprogress( sd, WIP_DISABLE_SKILLITEM ) || !sd->npc_item_flag || !( inf & INF_SELF_SKILL ) ){
#ifdef RENEWAL
			clif_msg( sd, WORK_IN_PROGRESS );
#endif
			return false;
		}
	}

	if ((pc_cant_act2(sd) || sd->chatID) &&
		skill_id != RK_REFRESH &&
		!( ( skill_id == SR_GENTLETOUCH_CURE || skill_id == SU_GROOMING ) && (sd->sc.opt1 == OPT1_STONE || sd->sc.opt1 == OPT1_FREEZE || sd->sc.opt1 == OPT1_STUN)) &&
		!(sd->state.storage_flag && (inf&INF_SELF_SKILL))) //SELF skills can be used with the storage open, issue: 8027
		return false;

	if( pc_issit(sd) )
		return false;

	if( skill_isNotOk(skill_id, sd) )
		return false;

	if( sd->bl.id != target_id && inf&INF_SELF_SKILL )
		target_id = sd->bl.id; // never trust the client

	if( target_id < 0 && -target_id == sd->bl.id ) // for disguises [Valaris]
		target_id = sd->bl.id;

	if( sd->ud.skilltimer != INVALID_TIMER ) {
		if( skill_id != SA_CASTCANCEL && skill_id != SO_SPELLFIST )
			return false;
	} else if( DIFF_TICK(tick, sd->ud.canact_tick) < 0 ) {
		if( sd->skillitem != skill_id ) {
			return false;
		}
	}

	if( sd->sc.option&OPTION_COSTUME )
		return false;

#ifndef RENEWAL
	if( sd->sc.getSCE(SC_BASILICA) && (skill_id != HP_BASILICA || sd->sc.getSCE(SC_BASILICA)->val4 != sd->bl.id) )
		return false; // On basilica only caster can use Basilica again to stop it.
#endif

	if( sd->menuskill_id ) {
		if( sd->menuskill_id == SA_TAMINGMONSTER ) {
			clif_menuskill_clear(sd); //Cancel pet capture.
		}else if( sd->menuskill_id == SG_FEEL ){
			clif_menuskill_clear( sd ); // Cancel selection
		}else if( sd->menuskill_id != SA_AUTOSPELL )
			return false; //Can't use skills while a menu is open.
	}

	if( sd->skillitem == skill_id ) {
		if( skill_lv != sd->skillitemlv )
			skill_lv = sd->skillitemlv;
		if( !(inf&INF_SELF_SKILL) )
			pc_delinvincibletimer(sd); // Target skills thru items cancel invincibility. [Inkfish]
		unit_skilluse_id(&sd->bl, target_id, skill_id, skill_lv);
		return false;
	}
	sd->skillitem = sd->skillitemlv = 0;

	if( SKILL_CHK_GUILD(skill_id) ) {
		if( sd->state.gmaster_flag || skill_id == GD_CHARGESHOUT_BEATING )
			skill_lv = guild_checkskill(sd->guild, skill_id);
		else
			skill_lv = 0;
	} else {
		if( skill_id != ALL_EQSWITCH ){
			skill_lv = min(pc_checkskill(sd, skill_id),skill_lv); //never trust client
		}
	}

	pc_delinvincibletimer(sd);

	if( skill_lv )
		return unit_skilluse_id(&sd->bl, target_id, skill_id, skill_lv);
	else
		return false;
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

static void autobuff_motion(struct map_session_data* sd, intptr_t data)
{
	if (pc_issit(sd) && status_isdead(&sd->bl)) {
		return;
	}
	int i = 0, n = 0, target_id = 0, spCost = 0, skill_id = 0, skill_lv = 0, skill_proc = 0, skill_size = 0;
	bool useSkill = false;
	std::vector<std::vector<int>>* m_autoskills = (std::vector<std::vector<int>>*)data;
	if ((skill_size = m_autoskills->size()) > 0) {

		i = rnd() % skill_size;
		for (n = 0; n < skill_size; i++, n++) {
			if (i == skill_size)
				i = 0;
			skill_id = m_autoskills->at(i)[0];
			if (!skill_db.find(skill_id))
				continue;
			sc_type sct = skill_get_sc(skill_id);


			skill_lv = min(m_autoskills->at(i)[1], skill_get_max(skill_id));
			skill_lv = min(skill_lv, pc_checkskill(sd, skill_id));
			if (sd->sc.getSCE(sct) || !skill_check_condition_castbegin_nocliffail(sd, skill_id, skill_lv))
				continue;
			useSkill = true;
			spCost = skill_get_sp(skill_id, skill_lv);
			if (useSkill && sd->battle_status.sp >= spCost && parse_skill_toid(sd, skill_id, skill_lv, sd->bl.id))
				break;
		}
	}

	return;
}


static void free_data_vector(intptr_t data) {
	std::vector<std::vector<int>>* v_autodata = (std::vector<std::vector<int>>*)data;
	delete v_autodata;
}
TIMER_FUNC(autobuff_timer) {
	struct map_session_data* sd = nullptr;
	if (!(sd = map_id2sd(id))) {
		free_data_vector(data);
		return 0;
	}

	if (!(sd->sc.getSCE(SC_AUTOBUFF))) {
		status_change_end(&sd->bl, SC_AUTOBUFF, INVALID_TIMER);
		free_data_vector(data);
		return 0;
	}
	autobuff_motion(sd, data);
	add_timer(gettick() + 200, autobuff_timer, sd->bl.id, data);

	return 1;
}


/// autobuff <autoskillid array variable>,<autoskilllv array variable>,<autoskillproc array variable>,<count>;
BUILDIN_FUNC(autobuff)
{
	const unsigned int total_parameters = 2;
	TBL_PC* sd = NULL;
	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;// no player attached
	if (sd->sc.getSCE(SC_AUTOBUFF)) {
		clif_showscript(&sd->bl,"Stop Autobuff !!",AREA);
		status_change_end(&sd->bl, SC_AUTOBUFF, INVALID_TIMER);
	} else {

		std::vector<std::vector<int>>* v_autobuffs = new std::vector<std::vector<int>>;
		if(!autoparse(st, total_parameters, v_autobuffs,0))
			return SCRIPT_CMD_FAILURE;
		clif_showscript(&sd->bl,"Autobuff !!",AREA);
		add_timer(gettick() + 200, autobuff_timer, sd->bl.id, (intptr_t)v_autobuffs);
		sc_start(&sd->bl, &sd->bl, SC_AUTOBUFF, 100, 0, gettick());
	}
	//clif_changeoption(&sd->bl);
	return SCRIPT_CMD_SUCCESS;
}

static bool shouldUsePot(map_session_data* sd,int pot_id) {
	bool use = true;
	switch (pot_id) {
			case ITEMID_ANODYNE:
				if(sd->sc.getSCE(SC_ENDURE) || !sd->sc.getSCE(SC_AUTOBUFF))
					use = false;
				if (skill_blockpc_get(sd, SM_ENDURE) != -1) {
					use = false;
				}
				break;
			case ITEMID_GLOOMY_BOX:
				if(sd->sc.getSCE(SC_CONCENTRATE) || sd->sc.getSCE(SC_QUAGMIRE) || !sd->sc.getSCE(SC_AUTOBUFF))
					use = false;
				break;
			case ITEMID_HOLY_WATER:
				if(!(sd->sc.getSCE(SC_CURSE)))
					use = false;
				break;
			case ITEMID_GREEN_POTION:
				if(!(sd->sc.getSCE(SC_POISON) || sd->sc.getSCE(SC_SILENCE) || sd->sc.getSCE(SC_BLIND) || sd->sc.getSCE(SC_CONFUSION)))
					use = false;
				break;
			case ITEMID_PANACEA:
			case ITEMID_ROYAL_JELLY:
				if (!(sd->sc.getSCE(SC_POISON) || sd->sc.getSCE(SC_SILENCE) || sd->sc.getSCE(SC_BLIND) || sd->sc.getSCE(SC_CONFUSION) || sd->sc.getSCE(SC_CURSE) || sd->sc.getSCE(SC_HALLUCINATION)))
					use=false;
				break;
			case ITEMID_ALOEBERA:
				if (sd->sc.getSCE(SC_PROVOKE) || status_get_element(&sd->bl) == ELE_UNDEAD || !sd->sc.getSCE(SC_AUTOBUFF))
					use = false;
				break;
			case ITEMID_BOX_OF_GRUDGE:
				if (sd->sc.getSCE(SC_ATKPOTION))
					use = false;
				break;
			case ITEMID_SLEEPY_BOX:
				if (sd->sc.getSCE(SC_MATKPOTION))
					use = false;
				break;
			case ITEMID_ABRASIVE:
				if (sd->sc.getSCE(SC_INCCRI))
					use = false;
				break;
			case ITEMID_LIGHT_CENTER_POT:
			case ITEMID_CENTER_POTION:
				if (sd->sc.getSCE(SC_ASPDPOTION0))
					use = false;
				break;
			case ITEMID_LIGHT_AWAKENING_POT:
			case ITEMID_AWAKENING_POTION:
				if (sd->sc.getSCE(SC_ASPDPOTION1))
					use = false;
				break;
			case ITEMID_LIGHT_BERSERK_POT:
			case ITEMID_BERSERK_POTION:
				if (sd->sc.getSCE(SC_ASPDPOTION2))
					use = false;
				break;
			case ITEMID_INSPECTOR_CERTIFICATE:
				if (sd->sc.getSCE(SC_SPEEDUP0))
					use = false;
				break;
			case ITEMID_ELEMENTAL_WATER:
			case ITEMID_ELEMENTAL_FIRE:
			case ITEMID_ELEMENTAL_EARTH:
			case ITEMID_ELEMENTAL_WIND:
			case ITEMID_WATER_OF_DARKNESS:
				if (sd->sc.getSCE(SC_ENCHANTARMS) || !sd->sc.getSCE(SC_AUTOBUFF))
					use = false;
				break;
			case ITEMID_RESIST_WATER:
				if (sd->sc.getSCE(SC_ARMOR_ELEMENT_WATER))
					use = false;
				break;
			case ITEMID_RESIST_WIND:
				if (sd->sc.getSCE(SC_ARMOR_ELEMENT_WIND))
					use = false;
				break;
			case ITEMID_RESIST_FIRE:
				if (sd->sc.getSCE(SC_ARMOR_ELEMENT_FIRE))
					use = false;
				break;
			case ITEMID_RESIST_EARTH:
				if (sd->sc.getSCE(SC_ARMOR_ELEMENT_EARTH))
					use = false;
				break;
			case ITEMID_AGI_DISH07:
			case ITEMID_AGI_DISH10:
				if (sd->sc.getSCE(SC_FOOD_AGI_CASH))
					use = false;
				break;
			case ITEMID_DEX_DISH07:
			case ITEMID_DEX_DISH10:
				if (sd->sc.getSCE(SC_FOOD_DEX_CASH))
					use = false;
				break;
			case ITEMID_STR_DISH07:
			case ITEMID_STR_DISH10:
				if (sd->sc.getSCE(SC_FOOD_STR_CASH))
					use = false;
				break;
			case ITEMID_VIT_DISH07:
			case ITEMID_VIT_DISH10:
				if (sd->sc.getSCE(SC_FOOD_VIT_CASH))
					use = false;
				break;
			case ITEMID_LUK_DISH07:
			case ITEMID_LUK_DISH10:
				if (sd->sc.getSCE(SC_FOOD_LUK_CASH))
					use = false;
				break;
			case ITEMID_INT_DISH07:
			case ITEMID_INT_DISH10:
				if (sd->sc.getSCE(SC_FOOD_INT_CASH))
					use = false;
				break;
			default:
				break;
		}
	return use;

}
static void autopot_use(struct map_session_data* sd, intptr_t data)
{
	if (status_isdead(&sd->bl)) {
		return;
	}
	std::vector<std::vector<int>>* v_autopot = (std::vector<std::vector<int>>*)data;
	for (int i = 0; i < v_autopot->size(); i++) {
		int j = 0,  pot_id = 0, pot_pos = 0, pot_threshold = 0, percent = 0;
		pot_id = v_autopot->at(i)[0];
		pot_threshold = v_autopot->at(i)[1];
		pot_pos = v_autopot->at(i)[2];
		std::shared_ptr<item_data> itmdb;
		if (!(itmdb=item_db.find(pot_id)))
			continue;
		switch (itmdb->nameid) {
			case ITEMID_BLUE_POTION:
			case ITEMID_LIGHT_BLUE_POT:
			case ITEMID_SIEGE_BLUE_POTION:
				percent=get_percentage(status_get_sp(&sd->bl), status_get_max_sp(&sd->bl));
				break;
			case ITEMID_YGGDRASILBERRY:
			case ITEMID_SEED_OF_YGGDRASIL:
				percent=min(get_percentage(status_get_hp(&sd->bl), status_get_max_hp(&sd->bl)),get_percentage(status_get_sp(&sd->bl), status_get_max_sp(&sd->bl)));
				break;
			default:
				percent=get_percentage(status_get_hp(&sd->bl), status_get_max_hp(&sd->bl));
				break;
		}
		if (!(percent < pot_threshold))
			continue;

		if (sd->inventory.u.items_inventory[pot_pos].nameid != pot_id) { //if position is empty(at first run it will always be)
			if ((pot_pos = pc_search_inventory(sd, pot_id)) < 0)
				continue;
			v_autopot->at(i)[2] = pot_pos;
		}
		status_change* sc;
		if (itmdb->delay.duration > 0 && itmdb->delay.sc >= SC_NONE && itmdb->delay.sc <= SC_MAX && (sc = &sd->sc) && sc->getSCE(itmdb->delay.sc))
			continue;
		if (!shouldUsePot(sd, pot_id))
			continue;
		if (pc_useitem(sd, pot_pos) > 0)
			break;

		/*sc_type sct = skill_get_sc(pot_id);
		if (sd->sc.getSCE(sct))
			continue;*/
	}

	return;
}

TIMER_FUNC(autopot_timer) {
	struct map_session_data* sd = nullptr;
	if (!(sd = map_id2sd(id))) {
		free_data_vector(data);
		return 0;
	}

	if (!(sd->sc.getSCE(SC_AUTOPOT))) {
		status_change_end(&sd->bl, SC_AUTOPOT, INVALID_TIMER);
		free_data_vector(data);
		return 0;
	}
	autopot_use(sd, data);
	add_timer(gettick() + 100, autopot_timer, sd->bl.id, data);

	return 1;
}


static int potPrio(int pot_id) {
	int p = 0;
	switch (pot_id) {
		case ITEMID_BLUE_POTION:
		case ITEMID_LIGHT_BLUE_POT:
		case ITEMID_SIEGE_BLUE_POTION:
			p = 2;
			break;
		case ITEMID_ALOEBERA:
		case ITEMID_BOX_OF_GRUDGE:
		case ITEMID_SLEEPY_BOX:
		case ITEMID_ABRASIVE:
		case ITEMID_LIGHT_CENTER_POT:
		case ITEMID_CENTER_POTION:
		case ITEMID_LIGHT_AWAKENING_POT:
		case ITEMID_AWAKENING_POTION:
		case ITEMID_LIGHT_BERSERK_POT:
		case ITEMID_BERSERK_POTION:
		case ITEMID_ELEMENTAL_FIRE:
		case ITEMID_ELEMENTAL_WATER:
		case ITEMID_ELEMENTAL_EARTH:
		case ITEMID_ELEMENTAL_WIND:
		case ITEMID_RESIST_WATER:
		case ITEMID_RESIST_WIND:
		case ITEMID_RESIST_FIRE:
		case ITEMID_RESIST_EARTH:
		case ITEMID_INSPECTOR_CERTIFICATE:
		case ITEMID_WATER_OF_DARKNESS:
		case ITEMID_GLOOMY_BOX:
		case ITEMID_ANODYNE:
			p = 1;
			break;
		case ITEMID_GREEN_POTION:
		case ITEMID_HOLY_WATER:
		case ITEMID_PANACEA:
		case ITEMID_ROYAL_JELLY:
			p = 4;
			break;
		case ITEMID_YGGDRASILBERRY:
		case ITEMID_SEED_OF_YGGDRASIL:
			p = 5;
			break;
		default:
			p = 3;
			break;
	}
	return p;
}

static bool comparePotPrio(std::vector<int> v1, std::vector<int> v2) {
	return potPrio(v1.at(0)) > potPrio(v2.at(0));
}
static void autopot_sort(std::vector<std::vector<int>>* v_autopot) {
	for (int i = 0; i < v_autopot->size(); i++) {
		int pot_id = 0;
		std::vector<int>& entry = v_autopot->at(i);
		pot_id = entry[0];
		std::shared_ptr<item_data> itmdb;
		if (!(itmdb = item_db.find(pot_id)))
			continue;
		sort(v_autopot->begin(), v_autopot->end(), comparePotPrio);
	}
	for (int i = 0; i < v_autopot->size(); ++i) {
		v_autopot->at(i).push_back(0); // set initial inventory position to 0;
	}
}

/// autopot <autopotid array variable>, <autopotthreshold array variable>,<count>;
BUILDIN_FUNC(autopot)
{
	const unsigned int total_parameters = 2;
	TBL_PC* sd = NULL;
	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;// no player attached
	if (sd->sc.getSCE(SC_AUTOPOT)) {
		clif_showscript(&sd->bl,"Stop Autopot !!",AREA);
		status_change_end(&sd->bl, SC_AUTOPOT, INVALID_TIMER);
	} else {

		std::vector<std::vector<int>>* v_autopot = new std::vector<std::vector<int>>;
		if(!autoparse(st, total_parameters, v_autopot))
			return SCRIPT_CMD_FAILURE;
		autopot_sort(v_autopot);
		clif_showscript(&sd->bl,"Autopot !!",AREA);
		add_timer(gettick() + 100, autopot_timer, sd->bl.id, (intptr_t)v_autopot);
		sc_start(&sd->bl, &sd->bl, SC_AUTOPOT, 100, 0, gettick());
	}
	//clif_changeoption(&sd->bl);
	return SCRIPT_CMD_SUCCESS;
}

static void autorestock_filter(std::vector<std::vector<int>>* v_autorestock) {
	for (int i = 0; i < v_autorestock->size(); i++) {
		int usable_id = 0;
		std::vector<int>& entry = v_autorestock->at(i);
		usable_id = entry[0];
		std::shared_ptr<item_data> itmdb;
		if (!(itmdb = item_db.find(usable_id)) || itmdb->type == IT_ARMOR || itmdb->type == IT_WEAPON)
			v_autorestock->erase(v_autorestock->begin() + i--);
	}
}
BUILDIN_FUNC(autorestock) {

	const unsigned int total_parameters = 2;
	TBL_PC* sd = NULL;
	if (!script_rid2sd(sd))
		return SCRIPT_CMD_SUCCESS;// no player attached
	if (sd->sc.getSCE(SC_AUTORESTOCK)) {
		clif_showscript(&sd->bl,"Stop Autorestock !!",AREA);
		status_change_end(&sd->bl, SC_AUTORESTOCK, INVALID_TIMER);
	} else {
		sd->autorestock.reset(new std::vector<std::vector<int>>);
		if(!autoparse_ignore_empty(st, total_parameters, sd->autorestock.get()))
			return SCRIPT_CMD_FAILURE;
		autorestock_filter(sd->autorestock.get());
		clif_showscript(&sd->bl,"Autorestock !!",AREA);
		sc_start(&sd->bl, &sd->bl, SC_AUTORESTOCK, 100, 0, gettick());
	}
	return SCRIPT_CMD_SUCCESS;
}


BUILDIN_FUNC(storeindex) {

	TBL_PC *sd;
	int i, amount;
	char* command = (char*)script_getfuncname(st);
	if( !script_accid2sd(4,sd) ){
		// In any case cancel script execution
		st->state = END;
		return SCRIPT_CMD_SUCCESS;
	}
	if (sd->state.storage_flag != 1)
	{	//Open storage.
		if( storage_storageopen(sd) == 1 ) {
			SCRIPT_CMD_SUCCESS;
		}
	}
	if (!script_isstring(st, 2)) {
		i = script_getnum(st, 2);// <item inventory index>
		amount = script_getnum(st, 3);
		if (amount <= 0)
			return SCRIPT_CMD_SUCCESS;// nothing to do
		if (sd->inventory.u.items_inventory[i].nameid > 0 && sd->inventory.u.items_inventory[i].amount > 0) {
			if (sd->inventory.u.items_inventory[i].amount < amount)
				amount -= sd->inventory.u.items_inventory[i].amount;
			sd->storage.dirty = true;
			if (storage_storageadd(sd, &sd->storage, i, amount) > 0) {
				storage_storageclose(sd);
				return SCRIPT_CMD_SUCCESS;
			}
		}
	}
	ShowError("buildin_%s: failed to add %d items in storage(AID=%d index=%u).\n", command, amount, sd->status.account_id,i);
	st->state = END;
	st->mes_active = 0;
	clif_scriptclose(sd, st->oid);
	return SCRIPT_CMD_FAILURE;
}

BUILDIN_FUNC(fillstorage) {
  TBL_PC *sd;
  int nameid, amount = 26999;
  item_data* data = nullptr;
  char* command = (char*)script_getfuncname(st);
  if (!script_accid2sd(4, sd)) {
    // In any case cancel script execution
    st->state = END;
    return SCRIPT_CMD_SUCCESS;
  }
  s_storage* stor = &sd->storage;
  std::vector<std::vector<int>>* fillers = new std::vector<std::vector<int>>;
  if (!autoparse(st, 1, fillers))
    return SCRIPT_CMD_FAILURE;
  for (int k = 0; k < fillers->size(); k++) {
    nameid = fillers->at(k)[0];
    data = itemdb_search(nameid);
    if (!itemdb_isstackable2(data)) { // Stackable
      continue;
    }
    int i;
    ARR_FIND(0, stor->max_amount, i, stor->u.items_storage[i].nameid == nameid);
    if (i < stor->max_amount) { // existing items found, stack them
      stor->u.items_storage[i].amount = amount;
      stor->dirty = true;
      continue;
    }

    //no existing item found, find free slot
    ARR_FIND(0, stor->max_amount, i, stor->u.items_storage[i].nameid == 0);
    if (i == stor->max_amount) {
      clif_displaymessage(sd->fd, "Storage is full...filling has been interrupted");
      st->state = END;
      return SCRIPT_CMD_FAILURE;
    }
    stor->u.items_storage[i].identify = 1;
    stor->u.items_storage[i].nameid = nameid;
    stor->amount++;
    stor->u.items_storage[i].amount = amount;
    stor->dirty = true;
  }

  clif_displaymessage(sd->fd,"Your storage was filled with various consumables");


  return SCRIPT_CMD_SUCCESS;
}
