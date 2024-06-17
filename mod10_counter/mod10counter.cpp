#include <systemc.h>
using namespace std;
#define NUM_COUNTERS 4
SC_MODULE(JK_FF)
{
    sc_in<bool> j;
    sc_in<bool> k;
    sc_out<bool> q;
    sc_out<bool> qn;
    sc_in<bool> clk;
    sc_in<bool> reset;
    bool state = 0;
    void handle_ff_state()
    {
        if (reset.read() == 1)
        {
            state = 0;
        }
        else
        {
            if (clk.posedge())
            {

                if (j == 0 && k == 0)
                {
                    // no change
                }
                if (j == 0 && k == 1)
                {
                    state = 0;
                }
                if (j == 1 && k == 0)
                {
                    state = 1;
                }
                if (j == 1 && k == 1)
                {
                    state = !state;
                }
            }
        }
        q.write(state);
        qn.write(!state);
    }

    SC_CTOR(JK_FF)
    {
        state = 0;
        SC_METHOD(handle_ff_state);
        sensitive << clk.pos();
        sensitive << reset;
    }
};

SC_MODULE(MOD10_COUNTER)
{
    JK_FF *jk_ff[NUM_COUNTERS];
    sc_signal<bool> j[NUM_COUNTERS];
    sc_signal<bool> k[NUM_COUNTERS];
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_signal<bool> q[NUM_COUNTERS];
    sc_signal<bool> qn[NUM_COUNTERS];
    sc_out<sc_uint<4>> count;

    void counter_logic()
    {
        j[0].write(1);
        k[0].write(1);
        j[1].write(qn[3] & q[0]);
        k[1].write(q[0]);
        j[2].write(q[1] & q[0]);
        k[2].write(q[1] & q[0]);
        j[3].write((q[0] & q[1] & q[2]));
        k[3].write(q[0]);
        sc_uint<4> cnt;
        for (int i = 0; i < NUM_COUNTERS; i++)
            cnt[i] = q[i];
        count.write(cnt);
    }

    SC_CTOR(MOD10_COUNTER)
    {
        for (int i = 0; i < NUM_COUNTERS; i++)
        {
            jk_ff[i] = new JK_FF(sc_gen_unique_name("jk_ff"));
            jk_ff[i]->j(j[i]);
            jk_ff[i]->k(k[i]);
            jk_ff[i]->clk(clk);
            jk_ff[i]->reset(reset);
            jk_ff[i]->q(q[i]);
            jk_ff[i]->qn(qn[i]);
        }

        SC_METHOD(counter_logic);
        for (int i = 0; i < NUM_COUNTERS; i++)
        {
            sensitive << q[i];
            sensitive << qn[i];
        }
    }

    ~MOD10_COUNTER()
    {
        for (int i = 0; i < NUM_COUNTERS; i++)
        {
            delete jk_ff[i];
        }
    }
};

SC_MODULE(Testbench)
{
    MOD10_COUNTER *mod10_cntr;
    sc_signal<bool> clk;
    sc_signal<bool> reset;
    sc_signal<sc_uint<4>> count;

    void generate_clock()
    {
        while(true){
            clk.write(0);
            wait(5, SC_NS);
            clk.write(1);
            wait(5, SC_NS);
        }
    }

    void monitor()
    {
        cout << "Timestamp: " << sc_time_stamp() << " | Count: " << count.read() << endl;
    }

    SC_CTOR(Testbench)
    {
        mod10_cntr = new MOD10_COUNTER("mod_10_counter");
        mod10_cntr->clk(clk);
        mod10_cntr->reset(reset);
        mod10_cntr->count(count);
        SC_THREAD(generate_clock);
        SC_METHOD(monitor);
        sensitive << clk.posedge_event();
    }
    ~Testbench()
    {
        delete mod10_cntr;
    }
};

int sc_main(int argc, char *argv[]) {
    Testbench tb("tb");
    sc_start(200, SC_NS);
    return 0;
};