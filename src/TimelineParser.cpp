#include "TimelineParser.h"
#include "WidgetComposition.h"
#include "GraphicsWidgetComposition.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetResources.h"

void TimelineParser::run() {

	int video_timeline_index = 0;

	// loop all elements currently in timeline
	for (int i = 0; i < composition->GetSegmentCount(); i++) {
		GraphicsWidgetSegment *p_segment = composition->GetSegment(i);

		if (p_segment) {
			for (int ii = 0; ii < p_segment->GetSequenceCount(); ii++) {
				GraphicsWidgetSequence *p_sequence = p_segment->GetSequence(ii);

				if (p_sequence && !p_sequence->IsEmpty() && p_sequence != nullptr) {

					for (int iii = 0; iii < p_sequence->GetResourceCount(); iii++) {
						AbstractGraphicsWidgetResource *p_resource = p_sequence->GetResource(iii);

						if (p_resource->type() == GraphicsWidgetVideoResourceType) {

							//WR Refresh timeline_index when parsing timeline, timeline_index is sent by signal CurrentVideoChanged to slot xPosChanged in mpPreview
							p_resource->timline_index = video_timeline_index++;
							// VideoResource found
							GraphicsWidgetVideoResource *timelineWidget = dynamic_cast<GraphicsWidgetVideoResource*>(p_resource);

							// create new playlist element
							VideoResource playlist_element;

							playlist_element.in = timelineWidget->GetEntryPoint().GetCount();
							playlist_element.RepeatCount = timelineWidget->GetRepeatCount();
							//playlist_element.Duration = timelineWidget->GetIntrinsicDuration().GetCount();
							//playlist_element.out = playlist_element.Duration * playlist_element.RepeatCount;
							playlist_element.Duration = timelineWidget->GetSourceDuration().GetCount();
							playlist_element.out = playlist_element.in + playlist_element.Duration * playlist_element.RepeatCount;

							if (p_resource->GetAsset()) {
								playlist_element.asset = p_resource->GetAsset();
							} // else: asset is invalid

							// add playlist element to playlist
							playlist->append(playlist_element);
						}
						else if (p_resource->type() == GraphicsWidgetSADMResourceType) {

							//WR Refresh timeline_index when parsing timeline, timeline_index is sent by signal CurrentVideoChanged to slot xPosChanged in mpPreview
							p_resource->timline_index = 0;
							// S-ADM found
							GraphicsWidgetSADMResource *timelineWidget = dynamic_cast<GraphicsWidgetSADMResource*>(p_resource);

							// Create Map entries
							if(!mMGASADMTracks->contains(ii + 1)) {
								mMGASADMTracks->insert(ii + 1, p_sequence->GetTrackId());
							}
						}
						else if (p_resource->type() == GraphicsWidgetTimedTextResourceType) {
							// TTML resource found
							GraphicsWidgetTimedTextResource *timelineWidget = dynamic_cast<GraphicsWidgetTimedTextResource*>(p_resource);

							TTMLParser *parser = new TTMLParser();

							// create new resource
							TTMLtimelineResource resource; // new resource
							resource.track_index = ii;
							resource.in = (float)timelineWidget->GetFirstVisibleFrame().GetSecondsF();
							resource.out = resource.in + (float)(timelineWidget->GetSourceDuration().GetCount() / timelineWidget->GetEditRate().GetQuotient());
							resource.RepeatCount = timelineWidget->GetRepeatCount();

							// check for other segments in track
							bool found = false; // default
							if (ttmls->count() > 0) {

								// check for other segments with same track id
								for (int z = ttmls->length(); z > 0; z--) {
									if (ttmls->at((z - 1)).track_index == ii) { // success!
										resource.timeline_in = ttmls->at((z - 1)).timeline_out;
										resource.timeline_out = (resource.out - resource.in) + resource.timeline_in;
										found = true;
										break; // exit loop
									}
								}
							}

							if (found == false) {
								resource.timeline_in = 0;
								resource.timeline_out = resource.out - resource.in;
							}
							
							// is asset alread wrapped in mxf?
							if (p_resource->GetAsset() && p_resource->GetAsset()->HasSourceFiles()) { // no
								// loop source files
								for (int z = 0; z < p_resource->GetAsset()->GetSourceFiles().length(); z++) {
									if (p_resource->InOutChanged) {
										parser->open(p_resource->GetAsset()->GetSourceFiles().at(z), resource, false);
									}
								}
							}
							else if (p_resource->GetAsset() && p_resource->InOutChanged) { // yes
								parser->open(p_resource->GetAsset()->GetPath().absoluteFilePath(), resource, true);
							}
							else { // ?
								// asset deleted?
							}

							if (resource.RepeatCount > 1) {
								resource.timeline_out += ((resource.out - resource.in) * (resource.RepeatCount - 1)); // update out-point
							}
#ifdef DEBUG_JP2K
					qDebug() << "resource.in" << resource.in << "resource.timeline_in" << resource.timeline_in
							<< "resource.out" << resource.out << "resource.timeline_out" << resource.timeline_out;
#endif

							parser->~TTMLParser();
		
							ttmls->append(resource);
						}
					}
				}
			}
		}
	}
	emit PlaylistFinished();
	this->thread()->quit();
}
