// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/blockchain/validate/validate_input.hpp>

#include <cstdint>
#include <kth/domain.hpp>

#ifdef WITH_CONSENSUS
#include <kth/consensus.hpp>
#endif

namespace kth {
namespace blockchain {

using namespace bc::chain;
using namespace bc::machine;

#ifdef WITH_CONSENSUS

using namespace bc::consensus;

// TODO: map bc policy flags.
uint32_t validate_input::convert_flags(uint32_t native_forks) {
    uint32_t flags = verify_flags_none;

    if (script::is_enabled(native_forks, rule_fork::bip16_rule)) {
        flags |= verify_flags_p2sh;
    }

    if (script::is_enabled(native_forks, rule_fork::bip65_rule)) {
        flags |= verify_flags_checklocktimeverify;
    }

    if (script::is_enabled(native_forks, rule_fork::bip66_rule)) {
        flags |= verify_flags_dersig;
    }

    if (script::is_enabled(native_forks, rule_fork::bip112_rule)) {
        flags |= verify_flags_checksequenceverify;
    }

#ifdef KTH_CURRENCY_BCH
    // BCH UAHF (FORKID on txns)
    if (script::is_enabled(native_forks, rule_fork::cash_verify_flags_script_enable_sighash_forkid)) {
        flags |= verify_flags_script_enable_sighash_forkid;
    }

    // Obligatory flags used on the 2017-Nov-13 BCH hard fork
    if (script::is_enabled(native_forks, rule_fork::cash_low_s_rule)) {
        flags |= verify_flags_low_s;
    }

    // // Obligatory flags used on the 2018-May-15 BCH hard fork
    // if (script::is_enabled(native_forks, rule_fork::cash_monolith_opcodes)) {
    //     flags |= verify_flags_script_enable_monolith_opcodes;
    // }

    // We make sure this node will have replay protection during the next hard fork.
    if (script::is_enabled(native_forks, rule_fork::cash_replay_protection)) {
        flags |= verify_flags_script_enable_replay_protection;
    }

    if (script::is_enabled(native_forks, rule_fork::cash_checkdatasig)) {
        flags |= verify_flags_script_enable_checkdatasig_sigops;
    }

    if (script::is_enabled(native_forks, rule_fork::cash_schnorr)) {
        flags |= verify_flags_script_script_enable_schnorr_multisig;
    }

    if (script::is_enabled(native_forks, rule_fork::cash_segwit_recovery)) {
        flags |= verify_flags_script_disallow_segwit_recovery;
    }

#else
    if (script::is_enabled(native_forks, rule_fork::bip141_rule)) {
        flags |= verify_flags_witness;
    }

    if (script::is_enabled(native_forks, rule_fork::bip147_rule)) {
        flags |= verify_flags_nulldummy;
    }
#endif
    return flags;
}

// TODO: map to corresponding bc::error codes.
code validate_input::convert_result(verify_result_type result) {
    switch (result)
    {
        // Logical true result.
        case verify_result_type::verify_result_eval_true:
            return error::success;

        // Logical false result.
        case verify_result_type::verify_result_eval_false:
            return error::stack_false;

        // Max size errors.
        case verify_result_type::verify_result_script_size:
        case verify_result_type::verify_result_push_size:
        case verify_result_type::verify_result_op_count:
        case verify_result_type::verify_result_stack_size:
        case verify_result_type::verify_result_sig_count:
        case verify_result_type::verify_result_pubkey_count:
            return error::invalid_script;

        // Failed verify operations.
        case verify_result_type::verify_result_verify:
        case verify_result_type::verify_result_equalverify:
        case verify_result_type::verify_result_checkmultisigverify:
        case verify_result_type::verify_result_checksigverify:
        case verify_result_type::verify_result_numequalverify:
            return error::invalid_script;

        // Logical/Format/Canonical errors.
        case verify_result_type::verify_result_bad_opcode:
        case verify_result_type::verify_result_disabled_opcode:
        case verify_result_type::verify_result_invalid_stack_operation:
        case verify_result_type::verify_result_invalid_altstack_operation:
        case verify_result_type::verify_result_unbalanced_conditional:
            return error::invalid_script;

        // Softfork safeness (should not see).
        case verify_result_type::verify_result_discourage_upgradable_nops:
            return error::operation_failed;
#ifndef KTH_CURRENCY_BCH
        case verify_result_type::verify_result_discourage_upgradable_witness_program:
            return error::operation_failed;

        // BIP66 errors (also BIP62, which is undeployed).
        case verify_result_type::verify_result_sig_der:
            return error::invalid_signature_encoding;
#endif
        // BIP62 errors (should not see).
        case verify_result_type::verify_result_sig_hashtype:
        case verify_result_type::verify_result_minimaldata:
        case verify_result_type::verify_result_sig_pushonly:
        case verify_result_type::verify_result_sig_high_s:

#if ! defined(KTH_CURRENCY_BCH)
        case verify_result_type::verify_result_sig_nulldummy:
#endif

        case verify_result_type::verify_result_pubkeytype:
        case verify_result_type::verify_result_cleanstack:
            return error::operation_failed_21;

        // BIP65/BIP112 (shared codes).
        case verify_result_type::verify_result_negative_locktime:
        case verify_result_type::verify_result_unsatisfied_locktime:
            return error::invalid_script;

        // Other errors.
        case verify_result_type::verify_result_op_return:
        case verify_result_type::verify_result_unknown_error:
            return error::invalid_script;

#ifndef KTH_CURRENCY_BCH
        // Segregated witness.
        case verify_result_type::verify_result_witness_program_wrong_length:
        case verify_result_type::verify_result_witness_program_empty_witness:
        case verify_result_type::verify_result_witness_program_mismatch:
        case verify_result_type::verify_result_witness_malleated:
        case verify_result_type::verify_result_witness_malleated_p2sh:
        case verify_result_type::verify_result_witness_unexpected:
        case verify_result_type::verify_result_witness_pubkeytype:
            return error::invalid_script;
#endif

        // Augmention codes for tx deserialization.
        case verify_result_type::verify_result_tx_invalid:
        case verify_result_type::verify_result_tx_size_invalid:
        case verify_result_type::verify_result_tx_input_invalid:
            return error::invalid_script;

        default:
            return error::invalid_script;
    }
}

// TODO: cache transaction wire serialization.
code validate_input::verify_script(const transaction& tx, uint32_t input_index,
    uint32_t branches) {

#ifdef KTH_CURRENCY_BCH
    bool witness = false;
#else
    bool witness = true;
#endif

    BITCOIN_ASSERT(input_index < tx.inputs().size());
    auto const& prevout = tx.inputs()[input_index].previous_output().validation;
    auto const script_data = prevout.cache.script().to_data(false);
    auto const prevout_value = prevout.cache.value();

    // auto const amount = bitcoin_cash ? prevout.cache.value() : 0;
    auto const amount = prevout.cache.value();
    // auto const prevout_value = prevout.cache.value();

    // Wire serialization is cached in support of large numbers of inputs.

    //TODO(fernando): implement KTH_CACHED_RPC_DATA (See domain) for the last parameter (unconfirmed = false).
    // auto const tx_data = tx.to_data(true, witness, false);
    auto const tx_data = tx.to_data(true, witness);

#ifdef KTH_CURRENCY_BCH
    auto res = consensus::verify_script(tx_data.data(),
        tx_data.size(), script_data.data(), script_data.size(), input_index,
        convert_flags(branches), amount);

    return convert_result(res);

#else // KTH_CURRENCY_BCH

    auto res = consensus::verify_script(tx_data.data(),
        tx_data.size(), script_data.data(), script_data.size(), amount,
        input_index, convert_flags(branches));

    return convert_result(res);

#endif // KTH_CURRENCY_BCH
}

#else //WITH_CONSENSUS

code validate_input::verify_script(transaction const& tx, uint32_t input_index, uint32_t forks) {

#error Not supported, build using -o with_consensus=True

    // if (bitcoin_cash) {
    //     return error::operation_failed_22;
    // }

    return script::verify(tx, input_index, forks);
}

#endif //WITH_CONSENSUS

} // namespace blockchain
} // namespace kth
