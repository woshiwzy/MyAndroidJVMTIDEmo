package com.sand.apm.jvmti;

import android.os.Bundle;
import android.util.Log;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import com.sand.apm.JVMTIHelper;

public class MainActivity extends AppCompatActivity {

    public String tag="jvmti";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        JVMTIHelper.init(this);
        JVMTIHelper.doCall();


        findViewById(R.id.buttonTestThread).setOnClickListener(v -> testThread());

        findViewById(R.id.buttonTest).setOnClickListener(v -> {
            Log.d(tag,"onclick envent");
            new String("123");
            System.gc();
            System.runFinalization();
        });

    }


    void testThread(){

        new Thread(() -> {
            Log.e(tag,"==========alloc callback.demo，即将分配新的内存");
            new String("HHHHH");
            try {
                Thread.sleep(5*1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            String x=new String("Hello");
        }).start();
    }

}