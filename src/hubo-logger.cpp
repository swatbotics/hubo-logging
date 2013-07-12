#include <hubo.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include "LogWriter.h"
#include <map>
#include <signal.h>

typedef std::pair<size_t, std::string> JointInfo;
typedef std::vector<JointInfo> JointInfoArray;

bool interrupted = false;

void buildJointTable(JointInfoArray& joints);
void interruptHandler(int sig);

bool useable(ach_status_t result) {
  return (result == ACH_OK || result == ACH_MISSED_FRAME);
}

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " OUTPUTFILE.data\n";
    return 1;
  }

  ach_channel_t chan_hubo_state;
  ach_channel_t chan_hubo_ref;

  ach_status_t r;

  r = ach_open(&chan_hubo_ref, HUBO_CHAN_REF_NAME , NULL);
  if (r != ACH_OK) {
    std::cerr << "error opening ref channel: " << ach_result_to_string(r) << "\n";
    return 1;
  }
  
  r = ach_open(&chan_hubo_state, HUBO_CHAN_STATE_NAME, NULL);
  if (r != ACH_OK) {
    std::cerr << "error opening state channel: " << ach_result_to_string(r) << "\n";
    return 1;
  }

  LogWriter writer(argv[1], 200);

  struct hubo_ref H_ref;
  struct hubo_state H_state;

  JointInfoArray jinfo;
  buildJointTable(jinfo);

  bool extra = false;

  writer.add(&H_state.time, "state.time", "s");

  for (size_t i=0; i<jinfo.size(); ++i) {

    size_t index = jinfo[i].first;
    const std::string& name = jinfo[i].second;
    writer.add(H_ref.ref+index, "ref.ref." + name, "rad");

    if (extra) {
      writer.add(H_ref.mode+index, "ref.mode." + name, "enum");
    }

    writer.add(&H_state.joint[index].ref, "state.joint." + name + ".ref", "rad");
    writer.add(&H_state.joint[index].pos, "state.joint." + name + ".pos", "rad");
    writer.add(&H_state.joint[index].cur, "state.joint." + name + ".cur", "amp");
    writer.add(&H_state.joint[index].vel, "state.joint." + name + ".vel", "rad/s");
    writer.add(&H_state.joint[index].duty, "state.joint." + name + ".duty");
    writer.add(&H_state.joint[index].heat, "state.joint." + name + ".heat", "J");
    writer.add(&H_state.joint[index].active, "state.joint." + name + ".active");
    writer.add(&H_state.joint[index].zeroed, "state.joint." + name + ".zeroed");

  }

  for (int i=0; i<4; ++i) {

    std::ostringstream ostr;
    ostr << "state.imu" << i;
    std::string imustr = ostr.str();

    writer.add(&H_state.imu[i].a_x, imustr+".a_x");
    writer.add(&H_state.imu[i].a_y, imustr+".a_y");
    writer.add(&H_state.imu[i].a_z, imustr+".a_z");

    writer.add(&H_state.imu[i].w_x, imustr+".w_x");
    writer.add(&H_state.imu[i].w_y, imustr+".w_y");
    writer.add(&H_state.imu[i].w_z, imustr+".w_z");

  }

  for (int i=0; i<4; ++i) {

    std::ostringstream ostr;
    ostr << "state.ft" << i;
    std::string ftstr = ostr.str();

    writer.add(&H_state.ft[i].m_x, ftstr+".m_x");
    writer.add(&H_state.ft[i].m_y, ftstr+".m_y");
    writer.add(&H_state.ft[i].f_z, ftstr+".f_z");

  }

  writer.sortChannels();

  writer.writeHeader();

  signal(SIGINT, interruptHandler);

  while (!interrupted) {

    bool write = false;
    size_t fs;

    r = ach_get( &chan_hubo_state, &H_state, sizeof(H_state), &fs, NULL, ACH_O_WAIT );
    if (useable(r)) {
      write = true;
    } else {
      std::cout << "warning: ach returned " << ach_result_to_string(r) << " for state\n";
    }

    r = ach_get( &chan_hubo_ref, &H_ref, sizeof(H_ref), &fs, NULL, ACH_O_LAST );
    if (useable(r)) { 
      write = true;
    } else {
      std::cout << "warning: ach returned " << ach_result_to_string(r) << " for ref\n";
    }

    if (write) {
      writer.writeSample();
    }

  }

  return 0;

}

void interruptHandler(int sig) {
  interrupted = true;
}

void buildJointTable(JointInfoArray& joints) {

  joints.clear();

#define ADD_JOINT(X) joints.push_back(std::make_pair(size_t((X)), (#X)))

  ADD_JOINT(RHY);
  ADD_JOINT(RHR);
  ADD_JOINT(RHP);
  ADD_JOINT(RKN);
  ADD_JOINT(RAP);
  ADD_JOINT(RAR);

  ADD_JOINT(LHY);
  ADD_JOINT(LHR);
  ADD_JOINT(LHP);
  ADD_JOINT(LKN);
  ADD_JOINT(LAP);
  ADD_JOINT(LAR);

  ADD_JOINT(RSP);
  ADD_JOINT(RSR);
  ADD_JOINT(RSY);
  ADD_JOINT(RSP);
  ADD_JOINT(REB);
  ADD_JOINT(RWY);
  ADD_JOINT(RWR);
  ADD_JOINT(RWP);

  ADD_JOINT(LSP);
  ADD_JOINT(LSR);
  ADD_JOINT(LSY);
  ADD_JOINT(LSP);
  ADD_JOINT(LEB);
  ADD_JOINT(LWY);
  ADD_JOINT(LWR);
  ADD_JOINT(LWP);

  ADD_JOINT(NKY);
  ADD_JOINT(NK1);
  ADD_JOINT(NK2);
  ADD_JOINT(WST);
  
  ADD_JOINT(RF1);
  ADD_JOINT(RF2);
  ADD_JOINT(RF3);
  ADD_JOINT(RF4);
  ADD_JOINT(RF5);

  ADD_JOINT(LF1);
  ADD_JOINT(LF2);
  ADD_JOINT(LF3);
  ADD_JOINT(LF4);
  ADD_JOINT(LF5);

#undef ADD_JOINT

}
