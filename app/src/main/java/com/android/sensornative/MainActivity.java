package com.android.sensornative;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;



public class MainActivity extends AppCompatActivity {
    private static final String TAG = "SENSORNATIVE";
    private  static boolean status = false;
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native String sensorStart();
    public native String sensorStop();

    public void sensorOperation(View view){
        Log.i(TAG, "SensorActivity sensorOperation");

        status = !status;
        if(status == true)
        {
            TextView tv = (TextView) findViewById(R.id.sample_text);
            tv.setText(sensorStart());
        }
        else
        {
            TextView tv = (TextView) findViewById(R.id.sample_text);
            tv.setText(sensorStop());
        }


        Button bt = (Button)findViewById(R.id.sensor_button);
        if(status == true)
        {
            bt.setText("Close");
        }
        else
        {
            bt.setText("Open");
        }
    }
}

