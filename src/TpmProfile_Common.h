/********************************************************************************/
/*										*/
/*						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2023				  	*/
/*										*/
/********************************************************************************/

// clang-format off
// clang-format off to preserve define alignment breaking sections.

// this file defines the common optional selections for the TPM library build
// Requires basic YES/NO defines are already set (by TpmBuildSwitches.h)
// Less frequently changed items are in other TpmProfile Headers.

#include <openssl/opensslconf.h>

#ifndef _TPM_PROFILE_COMMON_H_
#define _TPM_PROFILE_COMMON_H_
// YES & NO defined by TpmBuildSwitches.h
#if (YES != 1 || NO != 0)
#  error YES or NO incorrectly set
#endif
#if defined(ALG_YES) || defined(ALG_NO)
#  error ALG_YES and ALG_NO should only be defined by the TpmProfile_Common.h file
#endif

// Change these definitions to turn all algorithms ON or OFF. That is, to turn
// all algorithms on, set ALG_NO to YES. This is intended as a debug feature.
#define  ALG_YES                    YES
#define  ALG_NO                     NO

// Defines according to the processor being built for.
// Are building for a BIG_ENDIAN processor?
#ifndef BIG_ENDIAN_TPM
#define  BIG_ENDIAN_TPM             NO
#endif
#define  LITTLE_ENDIAN_TPM          !BIG_ENDIAN_TPM
// Does the processor put the most-significant bit at bit position 0?
#define MOST_SIGNIFICANT_BIT_0      NO
#define LEAST_SIGNIFICANT_BIT_0     !MOST_SIGNIFICANT_BIT_0
// Does processor support Auto align?
#define  AUTO_ALIGN                 NO

//***********************************************
// Defines for Symmetric Algorithms
//***********************************************

#define ALG_AES                     ALG_YES

#define     AES_128                     (YES * ALG_AES)
#define     AES_192                     (NO  * ALG_AES)
#define     AES_256                     (YES * ALG_AES)

#define ALG_SM4                     ALG_NO

#define     SM4_128                     (NO  * ALG_SM4)

#define ALG_CAMELLIA                ALG_YES
#ifdef OPENSSL_NO_CAMELLIA
#undef ALG_CAMELLIA
#define ALG_CAMELLIA                ALG_NO
#endif

#define     CAMELLIA_128                (YES * ALG_CAMELLIA)
#define     CAMELLIA_192                (NO  * ALG_CAMELLIA)
#define     CAMELLIA_256                (YES * ALG_CAMELLIA)

// must be yes if any above are yes.
#define ALG_SYMCIPHER               (ALG_AES || ALG_SM4 || ALG_CAMELLIA)
#define ALG_CMAC                    (YES * ALG_SYMCIPHER)

// block cipher modes
#define ALG_CTR                     ALG_YES
#define ALG_OFB                     ALG_YES
#define ALG_CBC                     ALG_YES
#define ALG_CFB                     ALG_YES
#define ALG_ECB                     ALG_YES

//***********************************************
// Defines for RSA Asymmetric Algorithms
//***********************************************
#define ALG_RSA                     ALG_YES
#define     RSA_1024                        (YES * ALG_RSA)
#define     RSA_2048                        (YES * ALG_RSA)
#define     RSA_3072                        (YES * ALG_RSA)
#define     RSA_4096                        (YES * ALG_RSA)
#define     RSA_16384                       (NO  * ALG_RSA)

#define     ALG_RSASSA                      (YES * ALG_RSA)
#define     ALG_RSAES                       (YES * ALG_RSA)
#define     ALG_RSAPSS                      (YES * ALG_RSA)
#define     ALG_OAEP                        (YES * ALG_RSA)

// RSA Implementation Styles
// use Chinese Remainder Theorem (5 prime) format for private key ?
#define CRT_FORMAT_RSA                  YES
#define RSA_DEFAULT_PUBLIC_EXPONENT     0x00010001

//***********************************************
// Defines for ECC Asymmetric Algorithms
//***********************************************
#define ALG_ECC                     ALG_YES
#define     ALG_ECDH                        (YES * ALG_ECC)
#define     ALG_ECDSA                       (YES * ALG_ECC)
#define     ALG_ECDAA                       (YES * ALG_ECC)
#define     ALG_SM2                         (YES * ALG_ECC)
#define     ALG_ECSCHNORR                   (YES * ALG_ECC)
#define     ALG_ECMQV                       (YES * ALG_ECC)
#define     ALG_KDF1_SP800_56A              (YES * ALG_ECC)
#define     ALG_EDDSA                       (NO  * ALG_ECC)
#define     ALG_EDDSA_PH                    (NO  * ALG_ECC)

#define     ECC_NIST_P192                   (YES * ALG_ECC)
#define     ECC_NIST_P224                   (YES * ALG_ECC)
#define     ECC_NIST_P256                   (YES * ALG_ECC)
#define     ECC_NIST_P384                   (YES * ALG_ECC)
#define     ECC_NIST_P521                   (YES * ALG_ECC)
#define     ECC_BN_P256                     (YES * ALG_ECC)
#define     ECC_BN_P638                     (YES * ALG_ECC)
#define     ECC_SM2_P256                    (YES * ALG_ECC)

#define     ECC_BP_P256_R1                  (NO * ALG_ECC)
#define     ECC_BP_P384_R1                  (NO * ALG_ECC)
#define     ECC_BP_P512_R1                  (NO * ALG_ECC)
#define     ECC_CURVE_25519                 (NO * ALG_ECC)
#define     ECC_CURVE_448                   (NO * ALG_ECC)

//***********************************************
// Defines for Hash/XOF Algorithms
//***********************************************
#define ALG_MGF1                            ALG_YES
#define ALG_SHA1                            ALG_YES
#define ALG_SHA256                          ALG_YES
#define ALG_SHA256_192                      ALG_NO
#define ALG_SHA384                          ALG_YES
#define ALG_SHA512                          ALG_YES

#define ALG_SHA3_256                        ALG_NO
#define ALG_SHA3_384                        ALG_NO
#define ALG_SHA3_512                        ALG_NO

#define ALG_SM3_256                         ALG_NO

#define ALG_SHAKE256_192                    ALG_NO
#define ALG_SHAKE256_256                    ALG_NO
#define ALG_SHAKE256_512                    ALG_NO

//***********************************************
// Defines for Stateful Signature Algorithms
//***********************************************
#define ALG_LMS                             ALG_NO
#define ALG_XMSS                            ALG_NO

//***********************************************
// Defines for Keyed Hashes
//***********************************************
#define ALG_KEYEDHASH                       ALG_YES
#define ALG_HMAC                            ALG_YES

//***********************************************
// Defines for KDFs
//***********************************************
#define ALG_KDF2                            ALG_YES
#define ALG_KDF1_SP800_108                  ALG_YES

//***********************************************
// Defines for Obscuration/MISC/compatibility
//***********************************************
#define ALG_XOR                             ALG_YES

//***********************************************
// Defines controlling ACT
//***********************************************
#define ACT_SUPPORT                         YES
#define RH_ACT_0                                (YES * ACT_SUPPORT)
#define RH_ACT_1                                ( NO * ACT_SUPPORT)
#define RH_ACT_2                                ( NO * ACT_SUPPORT)
#define RH_ACT_3                                ( NO * ACT_SUPPORT)
#define RH_ACT_4                                ( NO * ACT_SUPPORT)
#define RH_ACT_5                                ( NO * ACT_SUPPORT)
#define RH_ACT_6                                ( NO * ACT_SUPPORT)
#define RH_ACT_7                                ( NO * ACT_SUPPORT)
#define RH_ACT_8                                ( NO * ACT_SUPPORT)
#define RH_ACT_9                                ( NO * ACT_SUPPORT)
#define RH_ACT_A                                (YES * ACT_SUPPORT)
#define RH_ACT_B                                ( NO * ACT_SUPPORT)
#define RH_ACT_C                                ( NO * ACT_SUPPORT)
#define RH_ACT_D                                ( NO * ACT_SUPPORT)
#define RH_ACT_E                                ( NO * ACT_SUPPORT)
#define RH_ACT_F                                ( NO * ACT_SUPPORT)


//***********************************************
// Enable VENDOR_PERMANENT_AUTH_HANDLE?
//***********************************************
#define VENDOR_PERMANENT_AUTH_ENABLED       NO
// if YES, this must be valid per Part2 (TPM_RH_AUTH_00 - TPM_RH_AUTH_FF)
// if NO, this must be #undef
#undef  VENDOR_PERMANENT_AUTH_HANDLE

//***********************************************
// Defines controlling optional implementation
//***********************************************
#define FIELD_UPGRADE_IMPLEMENTED           NO

//***********************************************
// Buffer Sizes based on implementation
//***********************************************
// When using PC CRB, the page size for both commands and
// control registers is 4k.  The command buffer starts at
// offset 0x80, so the net size available is:
#define  MAX_COMMAND_SIZE               (4096-0x80)
#define  MAX_RESPONSE_SIZE              (4096-0x80)

//***********************************************
// Vendor Info
//***********************************************
// max buffer for vendor commands
// Max data buffer leaving space for TPM2B size prefix
#define VENDOR_COMMAND_COUNT          0
#define MAX_VENDOR_BUFFER_SIZE         (MAX_RESPONSE_SIZE-2)
#define PRIVATE_VENDOR_SPECIFIC_BYTES RSA_PRIVATE_SIZE

//***********************************************
// Defines controlling Firmware- and SVN-limited objects
//***********************************************
#define FW_LIMITED_SUPPORT                    YES
#define SVN_LIMITED_SUPPORT                   YES

//***********************************************
// Defines controlling External NV
//***********************************************
// This is a software reference implementation of the TPM: there is no
// "external NV" as such. This #define configures the TPM to implement
// "external NV" that is stored in the same place as "internal NV."
// NOTE: enabling this doesn't necessarily mean that the expanded
// (external-NV-specific) attributes are supported.
#define EXTERNAL_NV                           YES

#endif // _TPM_PROFILE_COMMON_H_
