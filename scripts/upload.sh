  #-F '_acuri=&#47;_ac&#47;update' \
# 192.168.1.144
IP=$1

curl -XPOST \
  -F '_acuri=/_ac/update' \
  -F bin=@./.pio/build/nodemcuv2/firmware.bin \
  "${IP}/_ac/update_act"

