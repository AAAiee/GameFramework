#pragma  once
#include <functional>

namespace Rubber {
	/** Manually driven timer with an owned callback; durations and update ts are seconds.
     * Supply a positive duration and nonnegative delta; inputs are not validated.
     * onUpdate invokes at most one timeout per call and subtracts one interval,
     * even when several elapsed. Callback executes inline and must not destroy
     * this timer. Pause retains elapsed time; resume continues; restart clears
     * elapsed/completion state but does not resume a paused timer. */
	class AnimationTimer {
	private:
		using onTimeOutFunc = std::function<void()>;
	public:
		AnimationTimer() = default;
		~AnimationTimer() = default;

		template<typename FuncType> 
		explicit AnimationTimer(const float durationPerRun, FuncType&& func ,bool loop = false)
			:m_IsOneTimeUse(!loop), m_DurationPerRun(durationPerRun), m_OnTimeOutCallback(std::forward<FuncType>(func)){ }

		void Pause();

		void resume();
		
		void restart();

		void setWaitTime( float durationPerRun);

		void onUpdate( float ts);

		void runOnlyOnce(bool flag);
		
		void setOnTimeOut(onTimeOutFunc&& timeout) {
			m_OnTimeOutCallback = std::move(timeout);
		}

	private:
		bool m_IsPaused = false;
		bool m_IsOneTimeUse = true;
		bool m_IsSingleRunEnded = false;
		float m_TimePassed = 0.0f;
		float m_DurationPerRun = 1.0f;
		onTimeOutFunc m_OnTimeOutCallback = nullptr;
	};


}