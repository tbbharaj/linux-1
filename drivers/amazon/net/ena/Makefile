#
# Makefile for the Elastic Network Adapter (ENA) device drivers.
#

obj-$(CONFIG_AMAZON_ENA) += ena.o

ena-y := ena_netdev.o ena_com.o ena_eth_com.o ena_ethtool.o

ena-$(CONFIG_SYSFS) += ena_sysfs.o
