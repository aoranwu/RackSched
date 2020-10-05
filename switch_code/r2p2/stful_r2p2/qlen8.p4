register server_qlen8 {
    width : 64;
    instance_count : CORE_NUM;
}

// increase the counter after finding the shortest_queue
blackbox stateful_alu salu_update_qlen8 {
    reg : server_qlen8;

    condition_lo : register_lo <= sq_md.round_idx;

    update_lo_1_predicate : condition_lo;
    update_lo_1_value : register_lo + 1;

    output_predicate : condition_lo;
    output_value : combined_predicate;
    output_dst : sq_md.to_qlen8;
}

action act_update_qlen8() {
    salu_update_qlen8.execute_stateful_alu(sq_md.core_idx);
}

table update_qlen8 {
    actions {act_update_qlen8;}
    default_action : act_update_qlen8();
}

// decrease the reply upon reply
blackbox stateful_alu salu_dcr_qlen8 {
    reg : server_qlen8;

    condition_lo : ipv4.srcAddr == HOSTIP_8;
    condition_hi : register_lo > 0;

    update_lo_1_predicate : condition_lo and condition_hi;
    update_lo_1_value : register_lo - 1;
}

action act_dcr_qlen8() {
    salu_dcr_qlen8.execute_stateful_alu(sq_reply.core_idx);
}

@pragma stage 8
table dcr_qlen8 {
    actions {act_dcr_qlen8;}
    default_action : act_dcr_qlen8();
}

action act_send_server8() {
    modify_field(sq_md.curserver,HOSTID_8);
}

table send_server8 {
    actions {act_send_server8;}
    default_action : act_send_server8();
}
