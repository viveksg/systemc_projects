#include <systemc.h>

using namespace std;

#define NUM_COUNTERS 4
SC_MODULE(JK_FF)
{
    // Ports
    sc_in<bool> j;
    sc_in<bool> k;
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_out<bool> q;
    sc_out<bool> q_n;

    bool state;
    void set_state()
    {
        if (reset.read() == 1)
            state = 0;
        else
        {
            if (clk.posedge())
            {
                if (j.read() == 0 && k.read() == 0)
                {
                    // no change
                }
                if (j.read() == 0 && k.read() == 1)
                {
                    state = 0; // reset
                }
                if (j.read() == 1 && k.read() == 0)
                {
                    state = 1; // set
                }
                if (j.read() == 1 && k.read() == 1)
                {
                    state = !state; // toggle
                }
            }
        }
        q.write(state);
        q_n.write(!state);
    }

    SC_CTOR(JK_FF)
    {
        state = 0;
        SC_METHOD(set_state);
        sensitive << clk.pos();
        sensitive << reset;
    }
};

SC_MODULE(UpCounter)
{
    sc_in<bool> clk;
    sc_in<bool> reset;
    sc_out<sc_uint<4>> count;
    sc_signal<bool> q[4], qn[4];
    sc_signal<bool> j[4], k[4];
    JK_FF *jk_ff[4];
    void counter_logic()
    {   
        //sc_signal<bool> ff2_inp;
        //ff2_inp.write(q[0] & q[1]);
        //sc_signal<bool> ff3_inp;
        //ff3_inp.write((q[0] & q[1]) & q[2]);
        j[0].write(1);
        k[0].write(1);
        j[1].write(q[0]);
        k[1].write(q[0]);
        j[2].write(q[0] & q[1]);
        k[2].write(q[0] & q[1]);
        j[3].write((q[0] & q[1]) & q[2]);
        k[3].write((q[0] & q[1]) & q[2]);

        sc_uint<4> cnt;
        for (int i = 0; i < 4; i++)
            cnt[i] = q[i].read();
        count.write(cnt);
    }

    SC_CTOR(UpCounter)
    {
        for (int i = 0; i < NUM_COUNTERS; i++)
        {
            jk_ff[i] = new JK_FF(sc_gen_unique_name("jk_ff"));
            jk_ff[i]->clk(clk);
            jk_ff[i]->reset(reset);
            jk_ff[i]->j(j[i]);
            jk_ff[i]->k(k[i]);
            jk_ff[i]->q(q[i]);
            jk_ff[i]->q_n(qn[i]);
        }

        SC_METHOD(counter_logic);
        for (int i = 0; i < 4; i++)
        {
            sensitive << q[i];
            sensitive << qn[i];
        }
    }

    ~UpCounter()
    {
        for (int i = 0; i < NUM_COUNTERS; i++)
        {
            delete jk_ff[i];
        }
    }
};

SC_MODULE(Testbench)
{
    UpCounter *upcntr;
    sc_signal<bool> clk;
    sc_signal<bool> reset;
    sc_signal<sc_uint<4>> count;
    void generate_clock()
    {
        while (true)
        {
            clk.write(0);
            wait(5, SC_NS);
            clk.write(1);
            wait(5, SC_NS);
        }
    }

    void reset_gen()
    {
       // reset.write(1);
       // wait(10, SC_NS);
        //reset.write(0);
    }

    void monitor()
    {
        cout << "Timestamp: " << sc_time_stamp() << " | Reset " << reset.read() << " | Count: " << count.read() << endl;
    }

    SC_CTOR(Testbench)
    {
        upcntr = new UpCounter("upcounter");
        upcntr->clk(clk);
        upcntr->reset(reset);
        upcntr->count(count);
        SC_THREAD(generate_clock);
        SC_THREAD(reset_gen);
        SC_METHOD(monitor);
        sensitive << clk.posedge_event();
    }

    ~Testbench()
    {
        delete upcntr;
    }
    
};

int sc_main(int argc, char *argv[])
{
    Testbench tb("tb");
    sc_start(200, SC_NS);
    return 0;
}