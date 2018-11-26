package james.dsp.service;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.AudioManager;
import android.media.audiofx.AudioEffect;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;
import james.dsp.R;
import james.dsp.activity.DSPManager;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import james.dsp.activity.JdspImpResToolbox;
/**
* <p>This calls listen to events that affect DSP function and responds to them.</p>
* <ol>
* <li>new audio session declarations</li>
* <li>headset plug / unplug events</li>
* <li>preference update events.</li>
* </ol>
*
* @author alankila
*/
public class HeadsetService extends Service
{
	/**
	* Helper class representing the full complement of effects attached to one
	* audio session.
	*
	* @author alankila
	*/
	Bitmap iconLarge;
	private class JDSPModule
	{
		private final UUID EFFECT_TYPE_NULL = UUID.fromString("ec7178ec-e5e1-4432-a3f4-4657e6795210");
		private final UUID EFFECT_JAMESDSP = UUID.fromString("f27317f4-c984-4de6-9a90-545759495bf2");
		public AudioEffect JamesDSP;
		private JDSPModule(int sessionId)
		{
			try
			{
				/*
				* AudioEffect constructor is not part of SDK. We use reflection
				* to access it.
				*/
				JamesDSP = AudioEffect.class.getConstructor(UUID.class,
					UUID.class, Integer.TYPE, Integer.TYPE).newInstance(
						EFFECT_TYPE_NULL, EFFECT_JAMESDSP, 0, sessionId);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
			if (DSPManager.devMsgDisplay)
			{
				if (JamesDSP != null)
				{
					AudioEffect.Descriptor dspDescriptor = JamesDSP.getDescriptor();
					if (dspDescriptor.uuid.equals(EFFECT_JAMESDSP))
						Toast.makeText(HeadsetService.this, "Effect name: " + dspDescriptor.name + "\nType id: " + dspDescriptor.type
							+ "\nUnique id: " + dspDescriptor.uuid + "\nImplementor: " + dspDescriptor.implementor, Toast.LENGTH_SHORT).show();
					else
						Toast.makeText(HeadsetService.this, "Effect load failed!\nTry re-enable audio effect in current player!", Toast.LENGTH_SHORT).show();
				}
			}
		}

		public void release()
		{
			JamesDSP.release();
		}

		public void recreateEffect(int sessionId)
		{
			JamesDSP.release();
			try
			{
				JamesDSP = AudioEffect.class.getConstructor(UUID.class, UUID.class, Integer.TYPE, Integer.TYPE).newInstance(
					EFFECT_TYPE_NULL, EFFECT_JAMESDSP, 0, sessionId);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}


		/**
		* Proxies call to AudioEffect.setParameter(byte[], byte[]) which is
		* available via reflection.
		*
		* @param audioEffect
		* @param parameter
		* @param value
		*/
		private byte[] IntToByte(int[] input)
		{
			int int_index, byte_index;
			int iterations = input.length;
			byte[] buffer = new byte[input.length * 4];
			int_index = byte_index = 0;
			for (; int_index != iterations;)
			{
				buffer[byte_index] = (byte)(input[int_index] & 0x00FF);
				buffer[byte_index + 1] = (byte)((input[int_index] & 0xFF00) >> 8);
				buffer[byte_index + 2] = (byte)((input[int_index] & 0xFF0000) >> 16);
				buffer[byte_index + 3] = (byte)((input[int_index] & 0xFF000000) >> 24);
				++int_index;
				byte_index += 4;
			}
			return buffer;
		}
		private int byteArrayToInt(byte[] encodedValue)
		{
			int value = (encodedValue[3] << 24);
			value |= (encodedValue[2] & 0xFF) << 16;
			value |= (encodedValue[1] & 0xFF) << 8;
			value |= (encodedValue[0] & 0xFF);
			return value;
		}
		private void setParameterIntArray(AudioEffect audioEffect, int parameter, int value[])
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = IntToByte(value);
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private void setParameterInt(AudioEffect audioEffect, int parameter, int value)
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[]
				{
					(byte)(value), (byte)(value >> 8),
					(byte)(value >> 16), (byte)(value >> 24)
				};
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private void setParameterShort(AudioEffect audioEffect, int parameter, short value)
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[]
				{
					(byte)(value), (byte)(value >> 8)
				};
				Method setParameter = AudioEffect.class.getMethod("setParameter", byte[].class, byte[].class);
				setParameter.invoke(audioEffect, arguments, result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
		private int getParameter(AudioEffect audioEffect, int parameter)
		{
			try
			{
				byte[] arguments = new byte[]
				{
					(byte)(parameter), (byte)(parameter >> 8),
					(byte)(parameter >> 16), (byte)(parameter >> 24)
				};
				byte[] result = new byte[4];
				Method getParameter = AudioEffect.class.getMethod("getParameter", byte[].class, byte[].class);
				getParameter.invoke(audioEffect, arguments, result);
				return byteArrayToInt(result);
			}
			catch (Exception e)
			{
				throw new RuntimeException(e);
			}
		}
	}

	public class LocalBinder extends Binder
	{
		public HeadsetService getService()
		{
			return HeadsetService.this;
		}
	}

	private final LocalBinder mBinder = new LocalBinder();

	/**
	* Known audio sessions and their associated audioeffect suites.
	*/
	protected final Map<Integer, JDSPModule> mAudioSessions = new HashMap<Integer, JDSPModule>();

	/**
	* Is a wired headset plugged in?
	*/
	public static boolean mUseHeadset;

	/**
	* Is bluetooth headset plugged in?
	*/
	public static boolean mUseBluetooth;

	private float[] mOverriddenEqualizerLevels;

	private int[] eqLevels = new int[10];
	/**
	* Receive new broadcast intents for adding DSP to session
	*/
	private final BroadcastReceiver mAudioSessionReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(Context context, Intent intent)
	{
		String action = intent.getAction();
		int sessionId = intent.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION, 0);
		if (action.equals(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION))
		{
			if (!mAudioSessions.containsKey(sessionId)) {
				JDSPModule fxId = new JDSPModule(sessionId);
				if (fxId.JamesDSP == null)
				{
					Log.e(DSPManager.TAG, "Audio session load fail");
					fxId.release();
					fxId = null;
				}
				else
					mAudioSessions.put(sessionId, fxId);
				updateDsp(false, true);
			}
		}
		if (action.equals(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION))
		{
			JDSPModule gone = mAudioSessions.remove(sessionId);
			if (gone != null)
				gone.release();
			gone = null;
		}
	}
	};
	/**
	* Update audio parameters when preferences have been updated.
	*/
	private final BroadcastReceiver mPreferenceUpdateReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(Context context, Intent intent)
	{
		updateDsp(false, true);
	}
	};

	/**
	* This code listens for changes in bluetooth and headset events. It is
	* adapted from google's own MusicFX application, so it's presumably the
	* most correct design there is for this problem.
	*/
	private final BroadcastReceiver mRoutingReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(final Context context, final Intent intent)
	{
		final String action = intent.getAction();
		final boolean prevUseHeadset = mUseHeadset;
		if (action.equals(AudioManager.ACTION_HEADSET_PLUG))
			mUseHeadset = intent.getIntExtra("state", 0) == 1;
		if (prevUseHeadset != mUseHeadset)
			updateDsp(true, false);
	}
	};

	private final BroadcastReceiver mBtReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(final Context context, final Intent intent)
	{
		final String action = intent.getAction();
		if (action.equals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED))
		{
			int state = intent.getIntExtra(BluetoothProfile.EXTRA_STATE,
				BluetoothProfile.STATE_CONNECTED);
			if (state == BluetoothProfile.STATE_CONNECTED && !mUseBluetooth)
			{
				mUseBluetooth = true;
				updateDsp(true, false);
			}
			else if (mUseBluetooth)
			{
				mUseBluetooth = false;
				updateDsp(true, false);
			}
		}
		else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED))
		{
			String stateExtra = BluetoothAdapter.EXTRA_STATE;
			int state = intent.getIntExtra(stateExtra, -1);
			if (state == BluetoothAdapter.STATE_OFF && mUseBluetooth)
			{
				mUseBluetooth = false;
				updateDsp(true, false);
			}
		}
	}
	};
	private void foregroundPersistent(String mFXType)
	{
		int mIconIDSmall = getResources().getIdentifier("ic_stat_icon", "drawable", getApplicationInfo().packageName);
		Intent notificationIntent = new Intent(this, DSPManager.class);
		PendingIntent contentItent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
		Notification statusNotify = new Notification.Builder(this)
			.setPriority(Notification.PRIORITY_MIN)
			.setAutoCancel(false)
			.setOngoing(true)
			.setDefaults(0)
			.setWhen(System.currentTimeMillis())
			.setSmallIcon(mIconIDSmall)
			.setLargeIcon(iconLarge)
			.setTicker("JamesDSP " + mFXType)
			.setContentTitle("JamesDSP")
			.setContentText(mFXType)
			.setContentIntent(contentItent)
			.build();
		startForeground(1, statusNotify);
	}
	@Override
		public void onCreate()
	{
		super.onCreate();
		IntentFilter audioFilter = new IntentFilter();
		audioFilter.addAction(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION);
		audioFilter.addAction(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION);
		registerReceiver(mAudioSessionReceiver, audioFilter);
		final IntentFilter intentFilter = new IntentFilter(AudioManager.ACTION_HEADSET_PLUG);
		registerReceiver(mRoutingReceiver, intentFilter);
		registerReceiver(mPreferenceUpdateReceiver, new IntentFilter(DSPManager.ACTION_UPDATE_PREFERENCES));
		final IntentFilter btFilter = new IntentFilter();
		btFilter.addAction(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
		btFilter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
		registerReceiver(mBtReceiver, btFilter);
		iconLarge = BitmapFactory.decodeResource(getResources(), R.drawable.icon);
		updateDsp(true, false);
	}

	@Override
		public void onDestroy()
	{
		super.onDestroy();
		unregisterReceiver(mAudioSessionReceiver);
		unregisterReceiver(mRoutingReceiver);
		unregisterReceiver(mPreferenceUpdateReceiver);
		stopForeground(true);
		if (iconLarge != null)
		{
			iconLarge.recycle();
			iconLarge = null;
		}
		mAudioSessions.clear();
	}

	@Override
		public IBinder onBind(Intent intent)
	{
		return mBinder;
	}

	/**
	* Gain temporary control over the global equalizer.
	* Used by DSPManager when testing a new equalizer setting.
	*
	* @param levels
	*/
	public void setEqualizerLevels(float[] levels)
	{
		mOverriddenEqualizerLevels = levels;
		updateDsp(false, false);
	}

	public static String getAudioOutputRouting()
	{
		if (mUseBluetooth)
			return "bluetooth";
		if (mUseHeadset)
			return "headset";
		return "speaker";
	}

	/**
	* Push new configuration to audio stack.
	*/
	protected void updateDsp(boolean notify, boolean updateConvolver)
	{
		final String mode = getAudioOutputRouting();
		SharedPreferences preferences = getSharedPreferences(DSPManager.SHARED_PREFERENCES_BASENAME + "." + mode, 0);
		if (notify)
		{
			if (mode == "bluetooth")
				foregroundPersistent(getString(R.string.bluetooth_title));
			else if (mode == "headset")
				foregroundPersistent(getString(R.string.headset_title));
			else
				foregroundPersistent(getString(R.string.speaker_title));
		}
		for (Integer sessionId : new ArrayList<Integer>(mAudioSessions.keySet()))
		{
			try
			{
				updateDsp(preferences, mAudioSessions.get(sessionId), updateConvolver, sessionId);
			}
			catch (Exception e)
			{
				mAudioSessions.remove(sessionId);
			}
		}
	}

	private void updateDsp(SharedPreferences preferences, JDSPModule session, boolean updateMajor, int sessionId)
	{
		boolean masterSwitch = preferences.getBoolean("dsp.masterswitch.enable", false);
		session.JamesDSP.setEnabled(masterSwitch); // Master switch
		if (masterSwitch)
		{
			int compressorEnabled = preferences.getBoolean("dsp.compression.enable", false) ? 1 : 0;
			int bassBoostEnabled = preferences.getBoolean("dsp.bass.enable", false) ? 1 : 0;
			int equalizerEnabled = preferences.getBoolean("dsp.tone.enable", false) ? 1 : 0;
			int reverbEnabled = preferences.getBoolean("dsp.headphone.enable", false) ? 1 : 0;
			int stereoWideEnabled = preferences.getBoolean("dsp.stereowide.enable", false) ? 1 : 0;
			int convolverEnabled = preferences.getBoolean("dsp.convolver.enable", false) ? 1 : 0;
			int dspModuleSamplingRate = session.getParameter(session.JamesDSP, 20000);
			if (dspModuleSamplingRate == 0) {
				Toast.makeText(HeadsetService.this, R.string.dspcrashed, Toast.LENGTH_LONG).show();
				session.recreateEffect(sessionId);
			}
			session.setParameterShort(session.JamesDSP, 1500, Short.valueOf(preferences.getString("dsp.masterswitch.clipmode", "1")));
			session.setParameterShort(session.JamesDSP, 1501, Short.valueOf(preferences.getString("dsp.masterswitch.finalgain", "100")));
			if (compressorEnabled == 1 && updateMajor)
			{
				session.setParameterShort(session.JamesDSP, 100, Short.valueOf(preferences.getString("dsp.compression.pregain", "0")));
				session.setParameterShort(session.JamesDSP, 101, Short.valueOf(preferences.getString("dsp.compression.threshold", "20")));
				session.setParameterShort(session.JamesDSP, 102, Short.valueOf(preferences.getString("dsp.compression.knee", "30")));
				session.setParameterShort(session.JamesDSP, 103, Short.valueOf(preferences.getString("dsp.compression.ratio", "12")));
				session.setParameterShort(session.JamesDSP, 104, Short.valueOf(preferences.getString("dsp.compression.attack", "30")));
				session.setParameterShort(session.JamesDSP, 105, Short.valueOf(preferences.getString("dsp.compression.release", "250")));
				session.setParameterShort(session.JamesDSP, 106, Short.valueOf(preferences.getString("dsp.compression.predelay", "60")));
			}
			session.setParameterShort(session.JamesDSP, 1200, (short)compressorEnabled); // Compressor switch
			if (bassBoostEnabled == 1 && updateMajor)
			{
				session.setParameterShort(session.JamesDSP, 112, Short.valueOf(preferences.getString("dsp.bass.mode", "80")));
				session.setParameterShort(session.JamesDSP, 113, Short.valueOf(preferences.getString("dsp.bass.filtertype", "0")));
				session.setParameterShort(session.JamesDSP, 114, Short.valueOf(preferences.getString("dsp.bass.freq", "55")));
			}
			session.setParameterShort(session.JamesDSP, 1201, (short)bassBoostEnabled); // Bass boost switch
			if (equalizerEnabled == 1)
			{
				/* Equalizer state is in a single string preference with all values separated by ; */
				if (mOverriddenEqualizerLevels != null)
				{
					for (short i = 0; i < mOverriddenEqualizerLevels.length; i++)
						eqLevels[i] = Math.round(mOverriddenEqualizerLevels[i] * 10000);
				}
				else
				{
					String[] levels = preferences.getString("dsp.tone.eq.custom", "0;0;0;0;0;0;0;0;0;0").split(";");
					for (short i = 0; i < levels.length; i++)
						eqLevels[i] = Math.round(Float.valueOf(levels[i]) * 10000);
				}
				session.setParameterIntArray(session.JamesDSP, 115, eqLevels);
			}
			session.setParameterShort(session.JamesDSP, 1202, (short)equalizerEnabled); // Equalizer switch
			if (reverbEnabled == 1 && updateMajor)
			{
				session.setParameterShort(session.JamesDSP, 127, Short.valueOf(preferences.getString("dsp.headphone.modeverb", "1")));
				session.setParameterShort(session.JamesDSP, 128, Short.valueOf(preferences.getString("dsp.headphone.preset", "0")));
				session.setParameterShort(session.JamesDSP, 129, Short.valueOf(preferences.getString("dsp.headphone.roomsize", "50")));
				session.setParameterShort(session.JamesDSP, 130, Short.valueOf(preferences.getString("dsp.headphone.reverbtime", "50")));
				session.setParameterShort(session.JamesDSP, 131, Short.valueOf(preferences.getString("dsp.headphone.damping", "50")));
				session.setParameterShort(session.JamesDSP, 133, Short.valueOf(preferences.getString("dsp.headphone.inbandwidth", "80")));
				session.setParameterShort(session.JamesDSP, 134, Short.valueOf(preferences.getString("dsp.headphone.earlyverb", "50")));
				session.setParameterShort(session.JamesDSP, 135, Short.valueOf(preferences.getString("dsp.headphone.tailverb", "50")));
			}
			session.setParameterShort(session.JamesDSP, 1203, (short)reverbEnabled); // Reverb switch
			if (stereoWideEnabled == 1 && updateMajor)
				session.setParameterShort(session.JamesDSP, 137, Short.valueOf(preferences.getString("dsp.stereowide.mode", "0")));
			session.setParameterShort(session.JamesDSP, 1204, (short)stereoWideEnabled); // Stereo widener switch
			if (convolverEnabled == 1 && updateMajor)
			{
				session.setParameterShort(session.JamesDSP, 1205, (short)0);
				String mConvIRFileName = preferences.getString("dsp.convolver.files", "");
				float quality = Float.valueOf(preferences.getString("dsp.convolver.quality", "1"));
				int[] impinfo = JdspImpResToolbox.GetLoadImpulseResponseInfo(mConvIRFileName);
				if (impinfo == null)
					Toast.makeText(HeadsetService.this, R.string.impfilefault, Toast.LENGTH_SHORT).show();
				else
				{
					if (impinfo[2] != dspModuleSamplingRate)
						Toast.makeText(HeadsetService.this, getString(R.string.unmatchedsamplerate, mConvIRFileName, impinfo[2], dspModuleSamplingRate), Toast.LENGTH_SHORT).show();
					else
					{
						int[] impulseResponse = JdspImpResToolbox.ReadImpulseResponseToInt();
						if (impulseResponse == null)
							Toast.makeText(HeadsetService.this, R.string.memoryallocatefail, Toast.LENGTH_LONG).show();
						else
						{
							int arraySize2Send = 4096;
							int impulseCutted = (int)(impulseResponse.length * quality);
							int[] sendArray = new int[arraySize2Send];
							int numTime2Send = (int)Math.ceil((double)impulseCutted / arraySize2Send); // Send number of times that have to send
							int[] sendBufInfo = new int[] {impulseCutted, impinfo[0], numTime2Send};
							session.setParameterIntArray(session.JamesDSP, 9999, sendBufInfo); // Send buffer info for module to allocate memory
							int[] finalArray = new int[numTime2Send*arraySize2Send]; // Fill final array with zero padding
							System.arraycopy(impulseResponse, 0, finalArray, 0, impulseCutted);
							for (int i = 0; i < numTime2Send; i++)
							{
								System.arraycopy(finalArray, arraySize2Send * i, sendArray, 0, arraySize2Send);
								session.setParameterShort(session.JamesDSP, 10003, (short)i); // Current increment
								session.setParameterIntArray(session.JamesDSP, 12000, sendArray); // Commit buffer
							}
							session.setParameterShort(session.JamesDSP, 10004, (short)1); // Notify send array completed and resize array in native side
							if (DSPManager.devMsgDisplay)
							{
								if (impinfo[0] == 2)
									Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName.replace(DSPManager.impulseResponsePath, ""), impinfo[2], impinfo[0], impinfo[1], (int)impulseCutted / 2), Toast.LENGTH_SHORT).show();
								else
									Toast.makeText(HeadsetService.this, getString(R.string.convolversuccess, mConvIRFileName.replace(DSPManager.impulseResponsePath, ""), impinfo[2], impinfo[0], impinfo[1], (int)impulseCutted), Toast.LENGTH_SHORT).show();
							}
						}
					}
				}
			}
			session.setParameterShort(session.JamesDSP, 1205, (short)convolverEnabled); // Convolver switch
		}
	}
}