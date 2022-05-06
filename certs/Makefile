# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel signature checking certificates.
#

obj-$(CONFIG_SYSTEM_TRUSTED_KEYRING) += system_keyring.o system_certificates.o common.o
obj-$(CONFIG_SYSTEM_BLACKLIST_KEYRING) += blacklist.o common.o
obj-$(CONFIG_SYSTEM_REVOCATION_LIST) += revocation_certificates.o
<<<<<<< HEAD
ifneq ($(CONFIG_SYSTEM_BLACKLIST_HASH_LIST),"")
=======
ifneq ($(CONFIG_SYSTEM_BLACKLIST_HASH_LIST),)
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
obj-$(CONFIG_SYSTEM_BLACKLIST_KEYRING) += blacklist_hashes.o
else
obj-$(CONFIG_SYSTEM_BLACKLIST_KEYRING) += blacklist_nohashes.o
endif

quiet_cmd_extract_certs  = CERT    $@
      cmd_extract_certs  = $(obj)/extract-cert $(extract-cert-in) $@
extract-cert-in = $(or $(filter-out $(obj)/extract-cert, $(real-prereqs)),"")

$(obj)/system_certificates.o: $(obj)/x509_certificate_list

$(obj)/x509_certificate_list: $(CONFIG_SYSTEM_TRUSTED_KEYS) $(obj)/extract-cert FORCE
	$(call if_changed,extract_certs)

targets += x509_certificate_list
<<<<<<< HEAD
$(obj)/x509_certificate_list: scripts/extract-cert $(SYSTEM_TRUSTED_KEYS_SRCPREFIX)$(SYSTEM_TRUSTED_KEYS_FILENAME) FORCE
	$(call if_changed,extract_certs,$(SYSTEM_TRUSTED_KEYS_SRCPREFIX)$(CONFIG_SYSTEM_TRUSTED_KEYS))
endif # CONFIG_SYSTEM_TRUSTED_KEYRING

clean-files := x509_certificate_list .x509.list x509_revocation_list
=======
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

# If module signing is requested, say by allyesconfig, but a key has not been
# supplied, then one will need to be generated to make sure the build does not
# fail and that the kernel may be used afterwards.
#
<<<<<<< HEAD
###############################################################################
ifndef CONFIG_MODULE_SIG_HASH
$(error Could not determine digest type to use from kernel config)
endif

redirect_openssl	= 2>&1
quiet_redirect_openssl	= 2>&1
silent_redirect_openssl = 2>/dev/null
openssl_available       = $(shell openssl help 2>/dev/null && echo yes)

# We do it this way rather than having a boolean option for enabling an
# external private key, because 'make randconfig' might enable such a
# boolean option and we unfortunately can't make it depend on !RANDCONFIG.
ifeq ($(CONFIG_MODULE_SIG_KEY),"certs/signing_key.pem")

ifeq ($(openssl_available),yes)
X509TEXT=$(shell openssl x509 -in "certs/signing_key.pem" -text 2>/dev/null)

$(if $(findstring rsaEncryption,$(X509TEXT)),,$(shell rm -f "certs/signing_key.pem"))
endif

$(obj)/signing_key.pem: $(obj)/x509.genkey
	@$(kecho) "###"
	@$(kecho) "### Now generating an X.509 key pair to be used for signing modules."
	@$(kecho) "###"
	@$(kecho) "### If this takes a long time, you might wish to run rngd in the"
	@$(kecho) "### background to keep the supply of entropy topped up.  It"
	@$(kecho) "### needs to be run as root, and uses a hardware random"
	@$(kecho) "### number generator if one is available."
	@$(kecho) "###"
	$(Q)openssl req -new -nodes -utf8 -$(CONFIG_MODULE_SIG_HASH) -days 36500 \
		-batch -x509 -config $(obj)/x509.genkey \
		-outform PEM -out $(obj)/signing_key.pem \
		-keyout $(obj)/signing_key.pem \
		$($(quiet)redirect_openssl)
	@$(kecho) "###"
	@$(kecho) "### Key pair generated."
	@$(kecho) "###"
=======
# We do it this way rather than having a boolean option for enabling an
# external private key, because 'make randconfig' might enable such a
# boolean option and we unfortunately can't make it depend on !RANDCONFIG.
ifeq ($(CONFIG_MODULE_SIG_KEY),certs/signing_key.pem)

keytype-$(CONFIG_MODULE_SIG_KEY_TYPE_ECDSA) := -newkey ec -pkeyopt ec_paramgen_curve:secp384r1

quiet_cmd_gen_key = GENKEY  $@
      cmd_gen_key = openssl req -new -nodes -utf8 -$(CONFIG_MODULE_SIG_HASH) -days 36500 \
		-batch -x509 -config $< \
		-outform PEM -out $@ -keyout $@ $(keytype-y) 2>&1

$(obj)/signing_key.pem: $(obj)/x509.genkey FORCE
	$(call if_changed,gen_key)
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a

targets += signing_key.pem

quiet_cmd_copy_x509_config = COPY    $@
      cmd_copy_x509_config = cat $(srctree)/$(src)/default_x509.genkey > $@

# You can provide your own config file. If not present, copy the default one.
$(obj)/x509.genkey:
	$(call cmd,copy_x509_config)

endif # CONFIG_MODULE_SIG_KEY

$(obj)/system_certificates.o: $(obj)/signing_key.x509

PKCS11_URI := $(filter pkcs11:%, $(CONFIG_MODULE_SIG_KEY))
ifdef PKCS11_URI
$(obj)/signing_key.x509: extract-cert-in := $(PKCS11_URI)
endif

$(obj)/signing_key.x509: $(filter-out $(PKCS11_URI),$(CONFIG_MODULE_SIG_KEY)) $(obj)/extract-cert FORCE
	$(call if_changed,extract_certs)

targets += signing_key.x509
<<<<<<< HEAD
$(obj)/signing_key.x509: scripts/extract-cert $(X509_DEP) FORCE
	$(call if_changed,extract_certs,$(MODULE_SIG_KEY_SRCPREFIX)$(CONFIG_MODULE_SIG_KEY))
endif # CONFIG_MODULE_SIG

ifeq ($(CONFIG_SYSTEM_REVOCATION_LIST),y)

$(eval $(call config_filename,SYSTEM_REVOCATION_KEYS))

$(obj)/revocation_certificates.o: $(obj)/x509_revocation_list

quiet_cmd_extract_certs  = EXTRACT_CERTS   $(patsubst "%",%,$(2))
      cmd_extract_certs  = scripts/extract-cert $(2) $@

targets += x509_revocation_list
$(obj)/x509_revocation_list: scripts/extract-cert $(SYSTEM_REVOCATION_KEYS_SRCPREFIX)$(SYSTEM_REVOCATION_KEYS_FILENAME) FORCE
	$(call if_changed,extract_certs,$(SYSTEM_REVOCATION_KEYS_SRCPREFIX)$(CONFIG_SYSTEM_REVOCATION_KEYS))
endif
=======

$(obj)/revocation_certificates.o: $(obj)/x509_revocation_list

$(obj)/x509_revocation_list: $(CONFIG_SYSTEM_REVOCATION_KEYS) $(obj)/extract-cert FORCE
	$(call if_changed,extract_certs)

targets += x509_revocation_list

hostprogs := extract-cert

HOSTCFLAGS_extract-cert.o = $(shell pkg-config --cflags libcrypto 2> /dev/null)
HOSTLDLIBS_extract-cert = $(shell pkg-config --libs libcrypto 2> /dev/null || echo -lcrypto)
>>>>>>> 672c0c5173427e6b3e2a9bbb7be51ceeec78093a
