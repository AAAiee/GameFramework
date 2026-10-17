#pragma  once
#include <array>

namespace Rubber {
	class GameLoopTimer {
	public:
		explicit GameLoopTimer(int targetFps = 60);
		~GameLoopTimer();

		int getFps() const;
		void startFrame();
		void waitForFrameEnd();
		// Seconds since startFrame(), including work and any wait so far.
		double getElapsedTime() const; 
		double getAverageFrameTime() const;
		// Seconds in the last completed waitForFrameEnd() sample, including pacing.
		// Before the first completed frame, returns the target frame duration.
		// This is a previous-frame sample, not time since the preceding call.
		double getDeltaTime()const ;

	private:
		// keep last 60 frames for smoothing
		static constexpr int MAX_FRAME_HISTORY = 60;
		// update FPS every second
		static constexpr double FPS_UPDATE_INTERVAL = 1.0;

		using Clock = std::chrono::steady_clock;
		using TimePoint = Clock::time_point;
		using Duration = std::chrono::duration<double>;

		TimePoint m_FrameStartTime;
		Duration m_FrameTargetDuration;
		int m_Frames;
		int m_CurrentFps;
		TimePoint m_LastFpsUpdate;
		Duration m_SleepError;
		bool m_IsInitialized;

		//circular buffer used to store all framesTime for the past UPDATE_INTERVAL
		std::array<double, MAX_FRAME_HISTORY>  m_FrameTimes; 
		int m_FrameTimeIndex;
		int m_FrameTimeCount;
	};

} 