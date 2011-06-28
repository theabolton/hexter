/*  nm hexter_fix.o |
 *    perl -n -e 'if (/ [CDT] _(.*)$/) { printf "#define %-40s FP_TAG(%s)\n", $1, $1; }' >>wrapper.h
 */

#ifndef HEXTER_USE_FLOATING_POINT

#define FP_TAG(x)  x##_fix

#else  /* HEXTER_USE_FLOATING_POINT */

#define FP_TAG(x)  x##_float

#endif  /* HEXTER_USE_FLOATING_POINT */

/* some things from config.h: */
#define VERSION " fptest"

/* in dx7_voice.c: */
#define dx7_eg_init_constants                    FP_TAG(dx7_eg_init_constants)
#define dx7_lfo_reset                            FP_TAG(dx7_lfo_reset)
#define dx7_lfo_set                              FP_TAG(dx7_lfo_set)
#define dx7_lfo_set_speed_x                      FP_TAG(dx7_lfo_set_speed_x)
#define dx7_lfo_update                           FP_TAG(dx7_lfo_update)
#define dx7_op_eg_set_increment                  FP_TAG(dx7_op_eg_set_increment)
#define dx7_op_eg_set_next_phase                 FP_TAG(dx7_op_eg_set_next_phase)
#define dx7_op_eg_set_phase                      FP_TAG(dx7_op_eg_set_phase)
#define dx7_op_envelope_prepare                  FP_TAG(dx7_op_envelope_prepare)
#define dx7_op_recalculate_increment             FP_TAG(dx7_op_recalculate_increment)
#define dx7_pitch_eg_set_increment               FP_TAG(dx7_pitch_eg_set_increment)
#define dx7_pitch_eg_set_next_phase              FP_TAG(dx7_pitch_eg_set_next_phase)
#define dx7_pitch_eg_set_phase                   FP_TAG(dx7_pitch_eg_set_phase)
#define dx7_pitch_envelope_prepare               FP_TAG(dx7_pitch_envelope_prepare)
#define dx7_portamento_prepare                   FP_TAG(dx7_portamento_prepare)
#define dx7_portamento_set_segment               FP_TAG(dx7_portamento_set_segment)
#define dx7_voice_calculate_runtime_parameters   FP_TAG(dx7_voice_calculate_runtime_parameters)
#define dx7_voice_new                            FP_TAG(dx7_voice_new)
#define dx7_voice_note_off                       FP_TAG(dx7_voice_note_off)
#define dx7_voice_note_on                        FP_TAG(dx7_voice_note_on)
#define dx7_voice_recalculate_freq_and_inc       FP_TAG(dx7_voice_recalculate_freq_and_inc)
#define dx7_voice_recalculate_frequency          FP_TAG(dx7_voice_recalculate_frequency)
#define dx7_voice_recalculate_volume             FP_TAG(dx7_voice_recalculate_volume)
#define dx7_voice_release_note                   FP_TAG(dx7_voice_release_note)
#define dx7_voice_set_data                       FP_TAG(dx7_voice_set_data)
#define dx7_voice_set_phase                      FP_TAG(dx7_voice_set_phase)
#define dx7_voice_setup_note                     FP_TAG(dx7_voice_setup_note)
#define dx7_voice_update_mod_depths              FP_TAG(dx7_voice_update_mod_depths)

/* in dx7_voice_data.c: */
#define base64                                   FP_TAG(base64)
#define decode_7in6                              FP_TAG(decode_7in6)
#define dssp_error_message                       FP_TAG(dssp_error_message)
#define dx7_bulk_dump_checksum                   FP_TAG(dx7_bulk_dump_checksum)
#define dx7_init_performance                     FP_TAG(dx7_init_performance)
#define dx7_patch_pack                           FP_TAG(dx7_patch_pack)
#define dx7_patch_unpack                         FP_TAG(dx7_patch_unpack)
#define dx7_patchbank_load                       FP_TAG(dx7_patchbank_load)
#define dx7_voice_copy_name                      FP_TAG(dx7_voice_copy_name)
#define dx7_voice_init_voice                     FP_TAG(dx7_voice_init_voice)
#define hexter_data_patches_init                 FP_TAG(hexter_data_patches_init)
#define hexter_data_performance_init             FP_TAG(hexter_data_performance_init)

/* in dx7_voice_render.c: */
#define dx7_voice_render                         FP_TAG(dx7_voice_render)

/* in dx7_voice_tables.c: */
#define dx7_voice_amd_to_ol_adjustment           FP_TAG(dx7_voice_amd_to_ol_adjustment)
#define dx7_voice_carrier_count                  FP_TAG(dx7_voice_carrier_count)
#define dx7_voice_carriers                       FP_TAG(dx7_voice_carriers)
#define dx7_voice_eg_ol_to_mod_index             FP_TAG(dx7_voice_eg_ol_to_mod_index)
#define dx7_voice_eg_ol_to_mod_index_table       FP_TAG(dx7_voice_eg_ol_to_mod_index_table)
#define dx7_voice_eg_rate_decay_duration         FP_TAG(dx7_voice_eg_rate_decay_duration)
#define dx7_voice_eg_rate_decay_percent          FP_TAG(dx7_voice_eg_rate_decay_percent)
#define dx7_voice_eg_rate_rise_duration          FP_TAG(dx7_voice_eg_rate_rise_duration)
#define dx7_voice_eg_rate_rise_percent           FP_TAG(dx7_voice_eg_rate_rise_percent)
#define dx7_voice_init_tables                    FP_TAG(dx7_voice_init_tables)
#define dx7_voice_lfo_frequency                  FP_TAG(dx7_voice_lfo_frequency)
#define dx7_voice_mss_to_ol_adjustment           FP_TAG(dx7_voice_mss_to_ol_adjustment)
#define dx7_voice_pitch_level_to_shift           FP_TAG(dx7_voice_pitch_level_to_shift)
#define dx7_voice_pms_to_semitones               FP_TAG(dx7_voice_pms_to_semitones)
#define dx7_voice_sin_table                      FP_TAG(dx7_voice_sin_table)
#define dx7_voice_velocity_ol_adjustment         FP_TAG(dx7_voice_velocity_ol_adjustment)

/* in hexter.c: */
#define dssi_descriptor                          FP_TAG(dssi_descriptor)
#define dssp_voicelist_mutex_lock                FP_TAG(dssp_voicelist_mutex_lock)
#define dssp_voicelist_mutex_unlock              FP_TAG(dssp_voicelist_mutex_unlock)
#define fini                                     FP_TAG(fini)
#define hexter_configure                         FP_TAG(hexter_configure)
#define hexter_deactivate                        FP_TAG(hexter_deactivate)
#define hexter_get_midi_controller               FP_TAG(hexter_get_midi_controller)
#define hexter_get_program                       FP_TAG(hexter_get_program)
#define hexter_select_program                    FP_TAG(hexter_select_program)
#define init                                     FP_TAG(init)
#define ladspa_descriptor                        FP_TAG(ladspa_descriptor)

/* in hexter_synth.c: */
#define dx7_voice_off                            FP_TAG(dx7_voice_off)
#define dx7_voice_start_voice                    FP_TAG(dx7_voice_start_voice)
#define hexter_instance_all_notes_off            FP_TAG(hexter_instance_all_notes_off)
#define hexter_instance_all_voices_off           FP_TAG(hexter_instance_all_voices_off)
#define hexter_instance_channel_pressure         FP_TAG(hexter_instance_channel_pressure)
#define hexter_instance_control_change           FP_TAG(hexter_instance_control_change)
#define hexter_instance_damp_voices              FP_TAG(hexter_instance_damp_voices)
#define hexter_instance_handle_edit_buffer       FP_TAG(hexter_instance_handle_edit_buffer)
#define hexter_instance_handle_monophonic        FP_TAG(hexter_instance_handle_monophonic)
// #define hexter_instance_handle_nrpn              FP_TAG(hexter_instance_handle_nrpn)
#define hexter_instance_handle_patches           FP_TAG(hexter_instance_handle_patches)
#define hexter_instance_handle_performance       FP_TAG(hexter_instance_handle_performance)
#define hexter_instance_handle_polyphony         FP_TAG(hexter_instance_handle_polyphony)
#define hexter_instance_init_controls            FP_TAG(hexter_instance_init_controls)
#define hexter_instance_key_pressure             FP_TAG(hexter_instance_key_pressure)
#define hexter_instance_note_off                 FP_TAG(hexter_instance_note_off)
#define hexter_instance_note_on                  FP_TAG(hexter_instance_note_on)
#define hexter_instance_pitch_bend               FP_TAG(hexter_instance_pitch_bend)
#define hexter_instance_select_program           FP_TAG(hexter_instance_select_program)
#define hexter_instance_set_performance_data     FP_TAG(hexter_instance_set_performance_data)
#define hexter_instance_set_program_descriptor   FP_TAG(hexter_instance_set_program_descriptor)
// #define hexter_instance_update_fc                FP_TAG(hexter_instance_update_fc)
// #define hexter_instance_update_op_param          FP_TAG(hexter_instance_update_op_param)
#define hexter_synth                             FP_TAG(hexter_synth)
#define hexter_synth_all_voices_off              FP_TAG(hexter_synth_all_voices_off)
#define hexter_synth_handle_global_polyphony     FP_TAG(hexter_synth_handle_global_polyphony)
#define hexter_synth_render_voices               FP_TAG(hexter_synth_render_voices)

