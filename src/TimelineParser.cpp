#include "TimelineParser.h"
#include "WidgetComposition.h"
#include "GraphicsWidgetComposition.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetResources.h"

void TimelineParser::run() {

	int last_track = 0;
	int track_index = 0;

	// loop all elements currently in timeline
	for (int i = 0; i < composition->GetSegmentCount(); i++) {
		GraphicsWidgetSegment *p_segment = composition->GetSegment(i);

		if (p_segment) {
			for (int ii = 0; ii < p_segment->GetSequenceCount(); ii++) {
				GraphicsWidgetSequence *p_sequence = p_segment->GetSequence(ii);

				if (p_sequence && !p_sequence->IsEmpty() && p_sequence != nullptr) {

					for (int iii = 0; iii < p_sequence->GetResourceCount(); iii++) {
						AbstractGraphicsWidgetResource *p_resource = p_sequence->GetResource(iii);

						//qDebug() << i << ii << iii;
						//qDebug() << p_resource->type();

						if (p_resource->type() == GraphicsWidgetVideoResourceType) {

							// VideoResource found
							GraphicsWidgetVideoResource *timelineWidget = dynamic_cast<GraphicsWidgetVideoResource*>(p_resource);

							// create new playlist element
							VideoResource playlist_element;
							
							playlist_element.in = timelineWidget->GetFirstVisibleFrame().GetOverallFrames();
							playlist_element.out = timelineWidget->GetLastVisibleFrame().GetOverallFrames();

							if (p_resource->GetAsset()) {
								playlist_element.asset = p_resource->GetAsset();
							} // else: asset is invalid

							// add playlist element to playlist
							playlist->append(playlist_element);
						}
						else if (p_resource->type() == GraphicsWidgetTimedTextResourceType) {
							// TTML resource found
							GraphicsWidgetTimedTextResource *timelineWidget = dynamic_cast<GraphicsWidgetTimedTextResource*>(p_resource);

							TTMLParser *parser = new TTMLParser();

							// check for new track
							if (ii != last_track && i == 0) {
								track_index++;
							}

							// create new segment
							TTMLtimelineResource resource; // new resource
							resource.track_index = ii;
							resource.in = (float)timelineWidget->GetFirstVisibleFrame().GetSecondsF();
							resource.out = (float)timelineWidget->GetLastVisibleFrame().GetSecondsF();

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
								resource.timeline_out = resource.out;
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

							parser->~TTMLParser();
		
							ttmls->append(resource);

							last_track = ii;
						}
					}
				}
			}
		}
	}

	emit PlaylistFinished();
	this->thread()->quit();
}
