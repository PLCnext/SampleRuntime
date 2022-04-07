#!/bin/sh

TARGET_IP="192.168.1.10"
TARGET_USER="admin"
TARGET_PASSWORD="123456"
APP="runtime"
REMOTE_PATH="/opt/plcnext/projects/runtime"


echo "Killing gdbserver, if it is running"
sshpass -p "${TARGET_PASSWORD}" ssh ${TARGET_USER}@${TARGET_IP} 'kill -9 `pidof gdbserver`'

echo "Stopping plcnext firmware\n"
sshpass -p "${TARGET_PASSWORD}" ssh ${TARGET_USER}@${TARGET_IP} '/etc/init.d/plcnext stop'

echo "Killing application, if it is running\n"
sshpass -p "${TARGET_PASSWORD}" ssh ${TARGET_USER}@${TARGET_IP} ${COMMAND} 'kill -9 `pidof '${APP}'`'

echo "Copy application to target\n"
# you can also copy a whole directory with the scp -r option, if the application
# consists of several files
sshpass -p "${TARGET_PASSWORD}" scp ${APP} ${TARGET_USER}@${TARGET_IP}:${REMOTE_PATH}/${APP}

echo "Set needed capabilities e.g. to execute realtime threads\n"
sshpass -p "${TARGET_PASSWORD}" ssh ${TARGET_USER}@${TARGET_IP} ${COMMAND} 'setcap cap_net_bind_service,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_sys_boot,cap_sys_nice,cap_sys_time+ep '${REMOTE_PATH}/${APP}

echo "Starting plcnext firmware\n"
sshpass -p "${TARGET_PASSWORD}" ssh ${TARGET_USER}@${TARGET_IP} '/etc/init.d/plcnext start'

sleep 3s

echo "Attaching gdbserver"
sshpass -p "${TARGET_PASSWORD}" ssh ${TARGET_USER}@${TARGET_IP} ${COMMAND} 'gdbserver :2345 --attach `pidof '${APP}'`'

echo "Finished"
