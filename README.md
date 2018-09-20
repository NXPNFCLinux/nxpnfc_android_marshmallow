# nxpnfc_android_marshmallow

This repository contains the files allowing to patch  6.0.1 Marshmallow version of AOSP source in order to add support for NXP-NCI NFC controllers (PN7120, PN7150) to an Android based system.
It only applies to android version later than Marshmallow (for Lollipop and Kitkat releases see appriopriate repositories: 
[Lollipop](https://github.com/NXPNFCLinux/nxpnfc_android_lollipop) / [Kitkat](https://github.com/NXPNFCLinux/nxpnfc_android_kitkat)).

Information about NXP NFC Controller can be found on [NXP website](https://www.nxp.com/products/identification-and-security/nfc/nfc-reader-ics:NFC-READER).

Further details about the stack and integration guidelines [here](https://github.com/NXPNFCLinux/nxpnfc_android_marshmallow/blob/master/AN11690%20-%20NXPNCI%20Android%20Porting%20Guidelines.pdf).

Release version
---------------
 * R1.2: Updated with PN7150 AGC debug and P2P active mode fixes
 * R1.1: Updated with HCE fix
 * R1.0: Initial release based on NFC_NCIHALx_AR3C.4.5.0_M_OpnSrc
