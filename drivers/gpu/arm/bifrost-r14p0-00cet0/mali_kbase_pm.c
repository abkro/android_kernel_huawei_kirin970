/*
 *
 * (C) COPYRIGHT 2010-2018 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */



/**
 * @file mali_kbase_pm.c
 * Base kernel power management APIs
 */

#include "mali_kbase.h"
#include "mali_midg_regmap.h"
#include "mali_kbase_vinstr.h"

#include "mali_kbase_pm.h"

int kbase_pm_powerup(struct kbase_device *kbdev, unsigned int flags)
{
	return kbase_hwaccess_pm_powerup(kbdev, flags);
}

void kbase_pm_halt(struct kbase_device *kbdev)
{
	kbase_hwaccess_pm_halt(kbdev);
}

void kbase_pm_context_active(struct kbase_device *kbdev)
{
	(void)kbase_pm_context_active_handle_suspend(kbdev, KBASE_PM_SUSPEND_HANDLER_NOT_POSSIBLE);
}

int kbase_pm_context_active_handle_suspend(struct kbase_device *kbdev, enum kbase_pm_suspend_handler suspend_handler)
{
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;
	int c;
	int old_count;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	/* Trace timeline information about how long it took to handle the decision
	 * to powerup. Sometimes the event might be missed due to reading the count
	 * outside of mutex, but this is necessary to get the trace timing
	 * correct. */
	old_count = kbdev->pm.active_count;
	if (old_count == 0)
		kbase_timeline_pm_send_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_ACTIVE);

	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);
	if (kbase_pm_is_suspending(kbdev)) {
		switch (suspend_handler) {
		case KBASE_PM_SUSPEND_HANDLER_DONT_REACTIVATE:
			if (kbdev->pm.active_count != 0)
				break;
			/* FALLTHROUGH */
		case KBASE_PM_SUSPEND_HANDLER_DONT_INCREASE:
			mutex_unlock(&kbdev->pm.lock);
			mutex_unlock(&js_devdata->runpool_mutex);
			if (old_count == 0)
				kbase_timeline_pm_handle_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_ACTIVE);
			return 1;

		case KBASE_PM_SUSPEND_HANDLER_NOT_POSSIBLE:
			/* FALLTHROUGH */
		default:
			KBASE_DEBUG_ASSERT_MSG(false, "unreachable");
			break;
		}
	}
	c = ++kbdev->pm.active_count;
	KBASE_TIMELINE_CONTEXT_ACTIVE(kbdev, c);
	KBASE_TRACE_ADD_REFCOUNT(kbdev, PM_CONTEXT_ACTIVE, NULL, NULL, 0u, c);

	/* Trace the event being handled */
	if (old_count == 0)
		kbase_timeline_pm_handle_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_ACTIVE);

	if (c == 1) {
		/* First context active: Power on the GPU and any cores requested by
		 * the policy */
		kbase_hwaccess_pm_gpu_active(kbdev);
	}
#if defined(CONFIG_DEVFREQ_THERMAL) && defined(CONFIG_MALI_DEVFREQ)
	if (kbdev->ipa.gpu_active_callback)
		kbdev->ipa.gpu_active_callback(kbdev->ipa.model_data);
#endif

	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);

	return 0;
}

KBASE_EXPORT_TEST_API(kbase_pm_context_active);

void kbase_pm_context_idle(struct kbase_device *kbdev)
{
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;
	int c;
	int old_count;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	/* Trace timeline information about how long it took to handle the decision
	 * to powerdown. Sometimes the event might be missed due to reading the
	 * count outside of mutex, but this is necessary to get the trace timing
	 * correct. */
	old_count = kbdev->pm.active_count;
	if (old_count == 0)
		kbase_timeline_pm_send_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_IDLE);

	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);

	c = --kbdev->pm.active_count;
	KBASE_TIMELINE_CONTEXT_ACTIVE(kbdev, c);
	KBASE_TRACE_ADD_REFCOUNT(kbdev, PM_CONTEXT_IDLE, NULL, NULL, 0u, c);

	KBASE_DEBUG_ASSERT(c >= 0);

	/* Trace the event being handled */
	if (old_count == 0)
		kbase_timeline_pm_handle_event(kbdev, KBASE_TIMELINE_PM_EVENT_GPU_IDLE);

	if (c == 0) {
		/* Last context has gone idle */
		kbase_hwaccess_pm_gpu_idle(kbdev);

		/* Wake up anyone waiting for this to become 0 (e.g. suspend). The
		 * waiters must synchronize with us by locking the pm.lock after
		 * waiting */
		wake_up(&kbdev->pm.zero_active_count_wait);
	}

#if defined(CONFIG_DEVFREQ_THERMAL) && defined(CONFIG_MALI_DEVFREQ)
	/* IPA may be using vinstr, in which case there may be one PM reference
	 * still held when all other contexts have left the GPU. Inform IPA that
	 * the GPU is now idle so that vinstr can drop it's reference.
	 *
	 * If the GPU was only briefly active then it might have gone idle
	 * before vinstr has taken a PM reference, meaning that active_count is
	 * zero. We still need to inform IPA in this case, so that vinstr can
	 * drop the PM reference and avoid keeping the GPU powered
	 * unnecessarily.
	 */
	if (c <= 1 && kbdev->ipa.gpu_idle_callback)
		kbdev->ipa.gpu_idle_callback(kbdev->ipa.model_data);
#endif

	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);
}

KBASE_EXPORT_TEST_API(kbase_pm_context_idle);

void kbase_pm_suspend(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev);

	/* Suspend vinstr.
	 * This call will block until vinstr is suspended. */
	kbase_vinstr_suspend(kbdev->vinstr_ctx);

	mutex_lock(&kbdev->pm.lock);
	KBASE_DEBUG_ASSERT(!kbase_pm_is_suspending(kbdev));
	kbdev->pm.suspending = true;
	mutex_unlock(&kbdev->pm.lock);

	/* From now on, the active count will drop towards zero. Sometimes, it'll
	 * go up briefly before going down again. However, once it reaches zero it
	 * will stay there - guaranteeing that we've idled all pm references */

	/* Suspend job scheduler and associated components, so that it releases all
	 * the PM active count references */
	kbasep_js_suspend(kbdev);

	/* Wait for the active count to reach zero. This is not the same as
	 * waiting for a power down, since not all policies power down when this
	 * reaches zero. */
	wait_event(kbdev->pm.zero_active_count_wait, kbdev->pm.active_count == 0);

	/* NOTE: We synchronize with anything that was just finishing a
	 * kbase_pm_context_idle() call by locking the pm.lock below */

	kbase_hwaccess_pm_suspend(kbdev);
}

void kbase_pm_resume(struct kbase_device *kbdev)
{
	/* MUST happen before any pm_context_active calls occur */
	kbase_hwaccess_pm_resume(kbdev);

	/* Initial active call, to power on the GPU/cores if needed */
	kbase_pm_context_active(kbdev);

	/* Resume any blocked atoms (which may cause contexts to be scheduled in
	 * and dependent atoms to run) */
	kbase_resume_suspended_soft_jobs(kbdev);

	/* Resume the Job Scheduler and associated components, and start running
	 * atoms */
	kbasep_js_resume(kbdev);

	/* Matching idle call, to power off the GPU/cores if we didn't actually
	 * need it and the policy doesn't want it on */
	kbase_pm_context_idle(kbdev);

	/* Resume vinstr operation */
	kbase_vinstr_resume(kbdev->vinstr_ctx);
}

